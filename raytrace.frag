#version 460 core

layout(location = 0) out vec3 FragColor;

#define INFINITY 2147483646
#define VERYSMALL 0.00000001

#define BVH_TYPE_BVH 0
#define BVH_TYPE_SPHERE 1

// Stuff sent from the CPU: -------------------------------------------------------------------

#define SPHERE_COUNT 1//{SPHERE_COUNT}
#define BVH_COUNT 1//{BVH_COUNT}

#define SAMPLES 1//{SAMPLES}
#define MAX_BOUNCES 1//{MAX_BOUNCES}

struct Camera {
    vec3 position;
    vec3 viewportTopLeft;
    vec3 du; // viewport vectors
    vec3 dv;
    vec3 backgroundColour;
    vec2 screenRes;
};

struct Material {
    vec3 colour;
    float reflective;
    float refractive;
    bool emitter;
};

struct BVHnode {
    vec3 AABBmin;
    vec3 AABBmax;
    int left_index;
    int right_index;
    int type;
};

struct Sphere {
    Material material;
    vec3 position;
    float radius;
};

layout (std140) uniform cameraBuffer {
    Camera camera;
};

layout (std140) uniform spheresBuffer {
    Sphere spheres[SPHERE_COUNT];
};

layout (std140) uniform bvhsBuffer {
    BVHnode bvhs[BVH_COUNT];
};

// Random float generation --------------------------------------------------------------------
// https://stackoverflow.com/a/17479300

uint hash (uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

// create a seed based on pixel coordinate
// TODO: also change each render
float seed = gl_FragCoord.x + (gl_FragCoord.y * camera.screenRes.x);
// float seed = 0.01;

float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

float random() {
    float v = floatConstruct(hash(floatBitsToUint(seed)));
    seed = v;
    return v;
}

float randomRange(float a, float b) {
    return a + random() * (b - a);
}

vec3 randomVec3(float a, float b) {
    return vec3(randomRange(a, b), randomRange(a, b), randomRange(a, b));
}

// Ray tracing --------------------------------------------------------------------------------

int nodeIndexStack[int(log2(BVH_COUNT)) + 1];

struct HitRecord {
    vec3 normal;
    vec3 p;
    float t;
    bool front_face;
    Material material;
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

vec3 pointAt(Ray ray, float t) {
    return ray.origin + ray.direction * t;
}

void setHitRecordNormal(inout HitRecord rec, Ray r, vec3 normal) {
    rec.front_face = dot(r.direction, normal) < 0;
    rec.normal = rec.front_face ? normal : -normal;
}

float magnitudeSquared(vec3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

vec3 randomUnitSphere() {
    while (true) {
        vec3 p = randomVec3(-1.0, 1.0);
        if (magnitudeSquared(p) < 1) {
            return p;
        }
    }
}

vec3 randomUnitVec3() {
    return normalize(randomUnitSphere());
}

vec3 randomHemisphereVec3(vec3 normal) {
    vec3 onSphere = randomUnitVec3();
    if (dot(onSphere, normal) > 0.0) {
        return onSphere;
    } else {
        return -onSphere;
    }
}

bool nearZero(vec3 v) {
    return all(lessThan(abs(v), vec3(VERYSMALL)));
}

vec3 refractVec3(vec3 v, vec3 n, float e) {
    float cosTheta = min(dot(-v, n), 1.0);
    vec3 perp = e * (v + cosTheta * n);
    vec3 par = -sqrt(abs(1.0 - magnitudeSquared(perp))) * n;
    return perp + par;
}

float reflectance(float cosine, float refIndex) {
    float r0 = (1 - refIndex) / (1 + refIndex);
    r0 = r0 * r0;
    return r0 + (1 - r0) * pow((1 - cosine), 5);
}

bool hitSphere(int sphereIndex, Ray r, float tmin, float tmax, inout HitRecord rec) {
    Sphere sphere = spheres[sphereIndex];

    vec3 oc = r.origin - sphere.position;
    float a = magnitudeSquared(r.direction);
    float halfb = dot(oc, r.direction);
    float c = magnitudeSquared(oc) - sphere.radius * sphere.radius;
    float discriminent = halfb * halfb - a * c;
    
    if (discriminent < 0) return false;

    float sqrtd = sqrt(discriminent);

    float root = (-halfb - sqrtd) / a;
    if (root <= tmin || tmax <= root) {
        root = (-halfb + sqrtd) / a;
        if (root <= tmin || tmax <= root) {
            return false;
        }
    }

    rec.t = root;
    rec.p = pointAt(r, rec.t);
    vec3 normal = (rec.p - sphere.position) / sphere.radius;
    setHitRecordNormal(rec, r, normal);
    rec.material = sphere.material;

    return true;
};

bool hitAABB(int AABBindex, Ray r, float tmin, float tmax) {
    BVHnode node = bvhs[AABBindex];
    vec3 invD = 1.0 / r.direction;

    float tminX = (node.AABBmin.x - r.origin.x) * invD.x;
    float tmaxX = (node.AABBmax.x - r.origin.x) * invD.x;

    if (tminX > tmaxX) {
        float temp = tminX;
        tminX = tmaxX;
        tmaxX = temp;
    }

    float tminY = (node.AABBmin.y - r.origin.y) * invD.y;
    float tmaxY = (node.AABBmax.y - r.origin.y) * invD.y;

    if (tminY > tmaxY) {
        float temp = tminY;
        tminY = tmaxY;
        tmaxY = temp;
    }

    if ((tminX > tmaxY) || (tminY > tmaxX)) {
        return false;
    }

    if (tminY > tminX) {
        tminX = tminY;
    }

    if (tmaxY < tmaxX) {
        tmaxX = tmaxY;
    }

    float tminZ = (node.AABBmin.z - r.origin.z) * invD.z;
    float tmaxZ = (node.AABBmax.z - r.origin.z) * invD.z;

    if (tminZ > tmaxZ) {
        float temp = tminZ;
        tminZ = tmaxZ;
        tmaxZ = temp;
    }

    if ((tminX > tmaxZ) || (tminZ > tmaxX)) {
        return false;
    }

    if (tminZ > tminX) {
        tminX = tminZ;
    }

    if (tmaxZ < tmaxX) {
        tmaxX = tmaxZ;
    }

    return (tminX < tmax) && (tmaxX > tmin);
}

bool hitWorld(Ray r, float tmin, float tmax, inout HitRecord rec) {
    HitRecord temp_rec;
    bool hitSomething = false;
    float closestSoFar = tmax;
    for (int i = 0; i < SPHERE_COUNT; i++) {
        if (hitSphere(i, r, tmin, closestSoFar, temp_rec)) {
            hitSomething = true;
            closestSoFar = temp_rec.t;
            rec = temp_rec;
        }
    }

    return hitSomething;
}

bool hitWorldFast(Ray r, float tmin, float tmax, inout HitRecord rec) {
    int stackPtr = 0;
    nodeIndexStack[stackPtr++] = 0;

    float closestSoFar = tmax;
    bool hitSomething = false;
	HitRecord temp_rec;

    while (stackPtr > 0) {
		int nodeIndex = nodeIndexStack[--stackPtr];
		BVHnode node = bvhs[nodeIndex];

		if (hitAABB(nodeIndex, r, tmin, closestSoFar)) {
			if (node.type == BVH_TYPE_SPHERE) {
				if (hitSphere(node.left_index, r, tmin, closestSoFar, temp_rec)) {
                    hitSomething = true;
					closestSoFar = temp_rec.t;
					rec = temp_rec;
				}
			}
			else {
				nodeIndexStack[stackPtr++] = node.left_index;
				nodeIndexStack[stackPtr++] = node.right_index;
			}
		}
	}

    return hitSomething;
}

vec3 getRayColour(in Ray ray) {
    vec3 colour = vec3(1, 1, 1);
    Ray currentRay = Ray(ray.origin, ray.direction);

    for (int i = 0; i < MAX_BOUNCES; i++) {
        HitRecord rec;
        if (hitWorldFast(currentRay, 0.001, INFINITY, rec)) {
            vec3 newDirection;

            if (rec.material.refractive > 0.0) { // refractive
                float refractionRatio = rec.front_face ? (1.0 / rec.material.refractive) : rec.material.refractive;
                vec3 unitDir = normalize(currentRay.direction);

                float cosTheta = min(dot(-unitDir, rec.normal), 1.0);
                float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

                bool cannotRefract = refractionRatio * sinTheta > 1.0;

                if (cannotRefract || reflectance(cosTheta, refractionRatio) > random()) {
                    newDirection = reflect(unitDir, rec.normal);
                }
                else {
                    newDirection = refract(unitDir, rec.normal, refractionRatio);
                }
            }
            else if (rec.material.reflective > 0.0) { // specular
                newDirection = reflect(normalize(currentRay.direction), rec.normal) + (1.0 - rec.material.reflective) * randomUnitVec3();
                if (dot(newDirection, rec.normal) < 0) {
                    colour = vec3(0, 0, 0);
                    break;
                };
            }

            else { // lambertian
                newDirection = rec.normal + randomUnitVec3();

                if (nearZero(newDirection)) {
                    newDirection = rec.normal;
                }
            }

            currentRay = Ray(rec.p, newDirection);
            colour *= rec.material.colour;

            continue;
        }

        vec3 unitDir = normalize(currentRay.direction);
        float a = 0.5 * unitDir.y + 1.0;
        colour *= (1.0 - a) * vec3(1, 1, 1) + a * vec3(0.5, 0.7, 1.0);
        break;
    }
    return colour;
};

vec3 getPixelSquare() {
    // random point in pixel
    float px = -0.5 + random();
    float py = -0.5 + random();
    return camera.du * px + camera.dv * py;
}

Ray getRay(in vec3 pos) {
    return Ray(camera.position, pos - camera.position);
}

void main() {
    // scale frag coords to nicer ones
    vec2 screenCoord = vec2(gl_FragCoord.x, gl_FragCoord.y);
    vec3 pixelCenter = camera.viewportTopLeft + screenCoord.x * camera.du + screenCoord.y * camera.dv;

    // antialiasing
    vec3 accumColour = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < SAMPLES; i++) {
        // fire sample ray at random point in pixel
        Ray r = getRay(pixelCenter + getPixelSquare());
        accumColour += getRayColour(r);
    }

    vec3 outColour = accumColour / SAMPLES;

    FragColor = sqrt(outColour);

}