#pragma once

#include <algorithm>

class HoldRepeater {
public:
	struct Config {
		float initialDelay = 0.5f;
		float startInterval = 0.1f;
		float minInterval = 0.05f;
		float rampTime = 1.0f;
	};

	explicit HoldRepeater(Config config) : config(config) {}
	HoldRepeater() = default;

	bool Update(float dt, bool held) {
		if (!held) {
			Reset();
			return false;
		}

		if (!wasHeldLastFrame) {
			wasHeldLastFrame = true;
			heldTime = 0.0f;
			timeSinceLastFire = 0.0f;
			return true;
		}

		heldTime += dt;
		timeSinceLastFire += dt;

		if (heldTime < config.initialDelay) return false;

		float interval = CurrentInterval();
		if (timeSinceLastFire >= interval) {
			timeSinceLastFire -= interval;
			return true;
		}
		return false;
	}

	void Reset() {
		wasHeldLastFrame = false;
		heldTime = 0.0f;
		timeSinceLastFire = 0.0f;
	}

private:
	float CurrentInterval() const {
		float t = heldTime - config.initialDelay;
		float ramp = (config.rampTime > 0.0f)
			? std::min(t / config.rampTime, 1.0f)
			: 1.0f;
		return config.startInterval + (config.minInterval - config.startInterval) * ramp;
	}

	Config config;
	bool wasHeldLastFrame = false;
	float heldTime = 0.0f;
	float timeSinceLastFire = 0.0f;
};