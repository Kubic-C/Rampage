#include "utility/ecs.hpp"
#include "transform.hpp"
#include "render/render.hpp"
#include "app.hpp"

int main() {
	logInit();

	App app("Rampage", 60);
	if (app.getStatus() == Status::CriticalError)
		return -1;

	return app.run();
}
