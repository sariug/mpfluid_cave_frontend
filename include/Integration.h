#ifndef MAGNUM_GLM_INTEGRATION_H
#define MAGNUM_GLM_INTEGRATION_H


#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/vector_relational.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Magnum/Math/RectangularMatrix.h>
#include <Magnum/Math/Quaternion.h>

namespace Magnum {
	namespace Math {
		namespace Implementation {

			// Magnum::VectorX <> glm::vecX 

			template<> struct VectorConverter <2, float, glm::vec2> {
				static Vector<2, Float> from(const glm::vec2& other) {
					return{ other.x, other.y };
				}

				static glm::vec2 to(const Vector<2, Float>& other) {
					return{ other[0], other[1] };
				}
			};

			template<> struct VectorConverter <3, float, glm::vec3> {
				static Vector<3, Float> from(const glm::vec3& other) {
					return{ other.x, other.y, other.z };
				}

				static glm::vec3 to(const Vector<3, Float>& other) {
					return{ other[0], other[1],  other[2] };
				}
			};

			template<> struct VectorConverter <4, float, glm::vec4> {
				static Vector<4, Float> from(const glm::vec4& other) {
					return{ other.x, other.y, other.z, other.w };
				}

				static glm::vec4 to(const Vector<4, Float>& other) {
					return{ other[0], other[1],  other[2], other[3] };
				}
			};

			//

			template<> struct VectorConverter <2, int, glm::ivec2> {
				static Vector<2, Int> from(const glm::ivec2& other) {
					return{ other.x, other.y };
				}

				static glm::ivec2 to(const Vector<2, Int>& other) {
					return{ other[0], other[1] };
				}
			};

			template<> struct VectorConverter <3, Int, glm::ivec3> {
				static Vector<3, Int> from(const glm::ivec3& other) {
					return{ other.x, other.y, other.z };
				}

				static glm::ivec3 to(const Vector<3, Int>& other) {
					return{ other[0], other[1],  other[2] };
				}
			};

			template<> struct VectorConverter <4, Int, glm::ivec4> {
				static Vector<4, Int> from(const glm::ivec4& other) {
					return{ other.x, other.y, other.z , other.w };
				}

				static glm::ivec4 to(const Vector<4, Int>& other) {
					return{ other[0], other[1],  other[2], other[3] };
				}
			};

			// Magnum::Matrix <> glm::mat

			template<> struct RectangularMatrixConverter<3, 3, float, glm::mat3> {
				static RectangularMatrix<3, 3, Float> from(const glm::mat3& other) {
					return{ Vector<3, Float>(other[0]),
						Vector<3, Float>(other[1]),
						Vector<3, Float>(other[2]) };
				}

				static glm::mat3 to(const RectangularMatrix<3, 3, Float>& other) {
					return{ other[0][0], other[0][1], other[0][2],
						other[1][0], other[1][1], other[1][2],
						other[2][0], other[2][1], other[2][2] };
				}
			};

			template<> struct RectangularMatrixConverter<4, 4, float, glm::mat4> {
				static RectangularMatrix<4, 4, Float> from(const glm::mat4& other) {
					return{ Vector<4, Float>(other[0]),
						Vector<4, Float>(other[1]),
						Vector<4, Float>(other[2]),
						Vector<4, Float>(other[3])};
				}

				static glm::mat4 to(const RectangularMatrix<4, 4, Float>& other) {
					return{ other[0][0], other[0][1], other[0][2], other[0][3],
						other[1][0], other[1][1], other[1][2], other[1][3],
						other[2][0], other[2][1], other[2][2], other[2][3],
						other[3][0], other[3][1], other[3][2], other[3][3]};
				}
			};

			// Magnum::Quaternion <> glm::quat
			// note:: glm elementwize constructor takes {wxyz} order

			template<> struct QuaternionConverter<float, glm::quat> {
				static Quaternion<Float> from(const glm::quat& other) {
					return{ { other.x, other.y, other.z }, other.w };
				}

				static glm::quat to(const Quaternion<Float>& other) {
					return{ other.scalar(), other.vector().x(), other.vector().y(), other.vector().z() };
				}
			};

		}
	}
}

#endif