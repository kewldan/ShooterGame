#ifndef SHOOTERGAME_COUNTER_H
#define SHOOTERGAME_COUNTER_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

class Counter {
	unsigned int i, perSecond, devider;
	double lastUpdate;
public:
	Counter(unsigned int devider) {
		i = 0;
		lastUpdate = glfwGetTime();
		perSecond = 0;
		this->devider = devider;
	};

	unsigned int getPerSecond() {
		if (glfwGetTime() - lastUpdate >= 1.f / devider) {
			perSecond = i * devider;
			i = 0;
			lastUpdate = glfwGetTime();
		}
		return perSecond;
	}

	void add() {
		i++;
	}
};

#endif