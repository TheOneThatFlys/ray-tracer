#include "Scene.h"
#include <algorithm>
#include <unordered_set>

class BVHNodeTemp {
public:
	glm::vec3 AABBmin;
	glm::vec3 AABBmax;
	std::shared_ptr<BVHNodeTemp> left, right;
	std::shared_ptr<SpheresBuffer> sphere;
	int type;

	BVHNodeTemp(const std::vector<std::shared_ptr<BVHNodeTemp>>& src_objects, size_t start, size_t end) {
		auto objects = src_objects;

		int axis = randomInt(0, 2);
		auto comparator = (axis == 0) ? box_compare_x 
						: (axis == 1) ? box_compare_y
					                  : box_compare_z;

		size_t span = end - start;

		if (span == 1) {
			left = right = objects[start];
		}
		else if (span == 2) {
			if (comparator(objects[start], objects[start + 1])) {
				left = objects[start];
				right = objects[start + 1];
			}
			else {
				left = objects[start + 1];
				right = objects[start];
			}
		}
		else {
			std::sort(objects.begin() + start, objects.begin() + end, comparator);
			auto mid = start + span / 2;
			left = std::make_shared<BVHNodeTemp>(objects, start, mid);
			right = std::make_shared<BVHNodeTemp>(objects, mid, end);
		}

		genAABB(*left, *right, AABBmin, AABBmax);
		type = BVH_TYPE_BVH;
	}

	BVHNodeTemp(SpheresBuffer& s) {
		glm::vec3 rvec(s.radius, s.radius, s.radius);
		glm::vec3 posa = s.position - rvec;
		glm::vec3 posb = s.position + rvec;
		AABBmin = glm::vec3(fmin(posa.x, posb.x), fmin(posa.y, posb.y), fmin(posa.z, posb.z));
		AABBmax = glm::vec3(fmax(posa.x, posb.x), fmax(posa.y, posb.y), fmax(posa.z, posb.z));
		type = BVH_TYPE_SPHERE;
		sphere = std::make_shared<SpheresBuffer>(s);
	}

	void genAABB(BVHNodeTemp a, BVHNodeTemp b, glm::vec3& _aabbmin, glm::vec3& _aabbmax) {
		_aabbmin = glm::vec3(fmin(a.AABBmin.x, b.AABBmin.x), fmin(a.AABBmin.y, b.AABBmin.y), fmin(a.AABBmin.z, b.AABBmin.z));
		_aabbmax = glm::vec3(fmax(a.AABBmax.x, b.AABBmax.x), fmax(a.AABBmax.y, b.AABBmax.y), fmax(a.AABBmax.z, b.AABBmax.z));
	}

	static bool box_compare_x(const std::shared_ptr<BVHNodeTemp> a, const std::shared_ptr<BVHNodeTemp> b) {
		return a->AABBmin.x < b->AABBmin.x;
	};

	static bool box_compare_y(const std::shared_ptr<BVHNodeTemp> a, const std::shared_ptr<BVHNodeTemp> b) {
		return a->AABBmin.y < b->AABBmin.y;
	};

	static bool box_compare_z(const std::shared_ptr<BVHNodeTemp> a, const std::shared_ptr<BVHNodeTemp> b) {
		return a->AABBmin.z < b->AABBmin.z;
	};

};

std::vector<BVHBuffer> createVector(std::shared_ptr<BVHNodeTemp> node) {
	BVHBuffer b;
	b.AABBmin = node->AABBmin;
	b.AABBmax = node->AABBmax;
	b.type = node->type;

	std::vector<BVHBuffer> bs;
	bs.push_back(b);

	if (b.type == BVH_TYPE_BVH) {
		auto left_v = createVector(node->left);
		auto right_v = createVector(node->right);
		bs.insert(bs.end(), left_v.begin(), left_v.end());
		bs.insert(bs.end(), right_v.begin(), right_v.end());
	}

	return bs;
};

static bool operator == (const BVHBuffer& buf, const BVHNodeTemp& node) {
	return (
		buf.AABBmin == node.AABBmin &&
		buf.AABBmax == node.AABBmax &&
		buf.type == node.type
		);
}

static std::ostream& operator << (std::ostream& os, const glm::vec3& v) {
	os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
	return os;
}

int getIndex(const std::vector<BVHBuffer>& bufs, const BVHNodeTemp& node, std::unordered_set<int>& exclude, const int headIndex = -1) {
	int i = -1;
	for (auto& b : bufs) {
		i++;
		if (headIndex == i) continue;
		if (b == node && exclude.count(i) == 0) {
			exclude.insert(i);
			return i;
		}
	}
	std::cout << "NOT GOOD";
}

int getIndex(const std::vector<SpheresBuffer>& bufs, const SpheresBuffer& s) {
	int i = 0;
	for (auto& b : bufs) {
		if (b.radius == s.radius && b.position == s.position) {
			return i;
		}
		i++;
	}
	std::cout << "NOT GOOD";
}

void printNode(const BVHNodeTemp& b) {
	std::cout << "TYPE: " << b.type << " | Left: " << b.left << " | Right: " << b.right << "\n";
}

void indexVector(std::vector<BVHBuffer>& bufs, const BVHNodeTemp& node, const std::vector<SpheresBuffer>& sphereBufs, std::unordered_set<int>& cacheHeadNodes, std::unordered_set<int>& cachePointerNodes) {
	int curNodeIndex = getIndex(bufs, node, cacheHeadNodes);
	if (bufs[curNodeIndex].type == BVH_TYPE_SPHERE) {
		int sphereIndex = getIndex(sphereBufs, *node.sphere);
		// std::cout << curNodeIndex << ": SPHERE-"<<sphereIndex<<"\n";
		bufs[curNodeIndex].left_index = sphereIndex;
		bufs[curNodeIndex].right_index = sphereIndex;
		return;
	};
	int leftIndex = getIndex(bufs, *node.left, cachePointerNodes, curNodeIndex);
	int rightIndex = getIndex(bufs, *node.right, cachePointerNodes, curNodeIndex);

	bufs[curNodeIndex].left_index = leftIndex;
	bufs[curNodeIndex].right_index = rightIndex;
	// std::cout << curNodeIndex << bufs[curNodeIndex].AABBmin << bufs[curNodeIndex].AABBmax <<": " << leftIndex << " | " << rightIndex << "\n";

	indexVector(bufs, *node.left, sphereBufs, cacheHeadNodes, cachePointerNodes);
	indexVector(bufs, *node.right, sphereBufs, cacheHeadNodes, cachePointerNodes);
}

Scene::Scene(int width, int height, int samples, int depth) {
	imageSize = glm::uvec2(width, height);

	shader = Shader("quad.vert", "raytrace.frag");
	textShader = Shader("passthrough.vert", "texture.frag");
	textShader.Create();
	CreateBalls();

	CalculateBVHs();

	shader.SetDefine("1//{SPHERE_COUNT}", (int)spheres.size());
	shader.SetDefine("1//{BVH_COUNT}", (int)bvhs.size());
	shader.SetDefine("1//{SAMPLES}", samples);
	shader.SetDefine("1//{MAX_BOUNCES}", depth);
	shader.Create();

	shader.Activate();

	CalculateViewport();

	createUniformBuffer(&cameraUBO, "cameraBuffer", 0, sizeof(CameraBuffer), &cameraBuf);
	createUniformBuffer(&spheresUBO, "spheresBuffer", 1, sizeof(SpheresBuffer) * spheres.size(), spheres.data());
	createUniformBuffer(&bvhUBO, "bvhsBuffer", 2, sizeof(BVHBuffer) * bvhs.size(), bvhs.data());

	textShader.Activate();

	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glGenTextures(1, &renderedTexture);

	glBindTexture(GL_TEXTURE_2D, renderedTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageSize.x, imageSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);
	glDrawBuffers(1, drawBuffers);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "UH OH!";
		exit(-1);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Scene::AddSphere(SpheresBuffer s, MaterialBuffer m) {
	s.material = m;
	spheres.push_back(s);
}

void Scene::CreateBalls() {
	camera.position = glm::vec3(13, 2, 3);
	camera.yaw = -173.5f;
	camera.pitch = -6.5f;
	camera.fov = 20.0;
	cameraBuf.backgroundColour = glm::vec3(0.1f, 0.1f, 0.1f);
	camera.LookAt(glm::vec3(0, 0, 0));

	camera.updateVectors();
	// floor
	AddSphere(
		SpheresBuffer(
			glm::vec3(0.0f, -1000.0f, 0.0f),
			1000.0f
		),
		MaterialBuffer(
			glm::vec3(0.5f, 0.5f, 0.5f),
			0.0f,
			0.0f
		)
	);

	const int n = 11;
	for (int a = -n; a < n; a++) {
		for (int b = -n; b < n; b++) {
			float c = randomFloat();
			glm::vec3 center(a + 0.9f * randomFloat(), 0.2f, b + 0.9f * randomFloat());

			if ((center - glm::vec3(4, 0.2f, 0)).length() > 0.9f) {
				MaterialBuffer mat;
				if (c < 0.8f) {
					mat.colour = randomVec3() * randomVec3();
				}
				else if (c < 0.95) {
					mat.colour = randomVec3() * 0.5f + glm::vec3(0.5f, 0.5f, 0.5f);
					mat.reflective = randomFloat() * 0.5f + 0.5f;
				}
				else {
					mat.refractive = 1.5f;
				}
				AddSphere(SpheresBuffer(center, 0.2f), mat);
			}
		}
	}

	AddSphere(SpheresBuffer(glm::vec3(0, 1, 0), 1.0f), MaterialBuffer(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 1.5f));
	AddSphere(SpheresBuffer(glm::vec3(0, 1, 0), -0.95f), MaterialBuffer(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 1.5f));

	AddSphere(SpheresBuffer(glm::vec3(-4, 1, 0), 1.0f), MaterialBuffer(glm::vec3(0.1f, 0.3f, 0.1f), 0.0f, 0.0f, true));

	AddSphere(SpheresBuffer(glm::vec3(4, 1, 0), 1.0f), MaterialBuffer(glm::vec3(0.7f, 0.6f, 0.5f), 1.0f, 0.0f));
}

void Scene::CalculateBVHs() {
	// std::cout<<spheres.size()<<"\n";
	std::vector<std::shared_ptr<BVHNodeTemp>> tempSpheres;
	for (auto& sphere : spheres) {
		auto s = std::make_shared<BVHNodeTemp>(sphere);
		tempSpheres.push_back(s);
	}
	BVHNodeTemp root = BVHNodeTemp(tempSpheres, 0, tempSpheres.size());

	bvhs = createVector(std::make_shared<BVHNodeTemp>(root));
	std::unordered_set<int> cacheHeadNodes, cacheLeafNodes;
	indexVector(bvhs, root, spheres, cacheHeadNodes, cacheLeafNodes);

	// [DEBUG] trial traversial
	//std::cout << "BVH SIZE: " << bvhs.size() << "\n";
	//std::cout << "SPHERE SIZE: " << spheres.size() << "\n";

	//int k = 0;
	//for (auto& node : bvhs) {
	//	if (node.type == BVH_TYPE_SPHERE) {
	//		k++;
	//		std::cout << "SPHERE: " << node.left_index << "\n";
	//	}
	//	else {
	//		std::cout << "NODE " << node.AABBmin << " " << node.AABBmax << ": " << node.left_index << " | " << node.right_index << "\n";
	//	}
	//}

	//std::cout << "BVH SPHERES: " << k << "\n";

	//int nodeIndexStack[100];
	//int stackPtr = 0;
	//nodeIndexStack[stackPtr++] = 0;
	//int n = 0;
	//while (stackPtr > 0) {
	//	int nodeIndex = nodeIndexStack[--stackPtr];
	//	BVHBuffer node = bvhs[nodeIndex];
	//	// std::cout << "NODE: " << nodeIndex << " | " << node.left_index << " | " << node.right_index << " | " << node.type << "\n";
	//	if (node.type == BVH_TYPE_SPHERE) {
	//		n++;
	//	}
	//	else {
	//		nodeIndexStack[stackPtr++] = node.left_index;
	//		nodeIndexStack[stackPtr++] = node.right_index;
	//	}
	//}

	//std::cout << "FOUND SPHERES: " << n << "\n";

	//std::cout << "DONE!\n\n\n\n";
}

void Scene::CalculateViewport() {
	float theta = glm::radians(camera.fov);
	float h = tan(theta / 2.0f);
	float viewportHeight = 2.0f * h * camera.focalLength;
	float viewportWidth = viewportHeight * ((float)imageSize.x / (float)imageSize.y);

	glm::vec3 w, u, v;
	w = -camera.direction;
	u = camera.rightv;
	v = camera.upv;

	glm::vec3 viewportU = viewportWidth * u;
	glm::vec3 viewportV = viewportHeight * v;

	cameraBuf.du = viewportU / (float) imageSize.x;
	cameraBuf.dv = viewportV / (float) imageSize.y;

	glm::vec3 viewportTopleft = cameraBuf.position - (camera.focalLength * w) - viewportU / 2.0f - viewportV / 2.0f;
	cameraBuf.viewportTopLeft = viewportTopleft + 0.5f * (cameraBuf.du + cameraBuf.dv);

	cameraBuf.position = camera.position;
	cameraBuf.screenRes = glm::vec2(imageSize);
}

void Scene::ResizeCallback(int width, int height) {
	imageSize.x = width; imageSize.y = height;
	glViewport(0, 0, width, height);
	CalculateViewport();
}

void Scene::Render() {
	glClear(GL_COLOR_BUFFER_BIT);

	shader.Activate();

	CalculateViewport();
	updateBuffer(cameraUBO, sizeof(cameraBuf), &cameraBuf);
	// updateBuffer(spheresUBO, sizeof(SpheresBuffer) * spheres.size(), spheres.data());

	quad.Render();
}

void Scene::TextureToScreen() {
	// glBlitFramebuffer(0, 0, imageSize.x, imageSize.y, 0, 0, imageSize.x, imageSize.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	textShader.Activate();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, imageSize.x, imageSize.y);
	quad.Render();
}

void Scene::RenderTextureInit() {
	CalculateViewport();
	updateBuffer(cameraUBO, sizeof(cameraBuf), &cameraBuf);
	updateBuffer(spheresUBO, sizeof(SpheresBuffer) * spheres.size(), spheres.data());
}

void Scene::RenderTexture(int startY, int endY) {
	shader.Activate();
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glViewport(0, startY, imageSize.x, endY - startY);
	quad.Render();
}

void Scene::Delete() {
	shader.Delete();
	quad.Delete();
}

void Scene::createUniformBuffer(GLuint* ubo, const char* name, int bindingPoint, size_t size, void* data) const {
	glGenBuffers(1, ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, *ubo);
	glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
	GLuint blockIndex = glGetUniformBlockIndex(shader.ID, name);
	if (blockIndex == 0xffffffff) {
		fprintf(stderr, "Invalid ubo block name '%s'", name);
		exit(1);
	}
	glUniformBlockBinding(shader.ID, blockIndex, bindingPoint);
	glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, *ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Scene::updateBuffer(GLuint ubo, size_t size, void* data) {
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}