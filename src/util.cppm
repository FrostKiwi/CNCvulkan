module;
#include <glm/glm.hpp>

export module util;

export template <int Length>
	requires(Length == 3 || Length == 4)
auto hsv_to_rgb(glm::vec<Length, float> hsv) -> glm::vec<Length, float> {
	glm::vec3 rgb {
		std::fabs(hsv.x * 6.0f - 3.0f) - 1.0f,
		2.0f - std::fabs(hsv.x * 6.0f - 2.0f),
		2.0f - std::fabs(hsv.x * 6.0f - 4.0f)
	};
	rgb = glm::clamp(rgb, 0.0f, 1.0f);
	rgb = ((rgb - 1.0f) * hsv.y + 1.0f) * hsv.z;

	if constexpr (Length == 4)
		return glm::vec4 {rgb, hsv.w};
	else
		return rgb;
}
