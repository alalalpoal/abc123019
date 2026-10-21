#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

// Fixed buffer: 4 frames AT 60 FPS = 1/15 s (~66.7 ms), measured in real time.
static constexpr float BUFFER_SECONDS = 4.f / 60.f;

namespace {
	struct Pending {
		bool active = false;
		bool released = false;
		float age = 0.f;
		bool releaseNext = false;
	};

	Pending g_pending[2];
	bool g_firing = false;

	bool canAct(PlayerObject* p) {
		if (!p) return false;
		if (p->m_isShip || p->m_isDart || p->m_isSwing) return true;
		return p->m_isOnGround || p->m_touchedRing;
	}

	bool classicPlay(GJBaseGameLayer* layer) {
		if (!typeinfo_cast<PlayLayer*>(layer)) return false;
		if (layer->m_levelSettings && layer->m_levelSettings->m_platformerMode) return false;
		return true;
	}
}

class $modify(FFBGameLayer, GJBaseGameLayer) {
	void handleButton(bool down, int button, bool isPlayer1) {
		if (!g_firing && button == 1 && classicPlay(this)) {
			auto& st = g_pending[isPlayer1 ? 0 : 1];
			auto* player = isPlayer1 ? m_player1 : m_player2;
			if (down) {
				st = Pending{};
				if (!canAct(player)) st.active = true;
			}
			else if (st.active) {
				st.released = true;
			}
		}
		GJBaseGameLayer::handleButton(down, button, isPlayer1);
	}

	void update(float dt) {
		if (classicPlay(this)) {
			for (int i = 0; i < 2; ++i) {
				auto& st = g_pending[i];
				bool isP1 = (i == 0);
				auto* player = isP1 ? m_player1 : m_player2;

				if (st.releaseNext) {
					g_firing = true;
					GJBaseGameLayer::handleButton(false, 1, isP1);
					g_firing = false;
					st = Pending{};
					continue;
				}
				if (!st.active) continue;

				st.age += dt;
				if (st.age > BUFFER_SECONDS) {
					st = Pending{};
					continue;
				}
				if (st.released && canAct(player)) {
					g_firing = true;
					GJBaseGameLayer::handleButton(true, 1, isP1);
					g_firing = false;
					st.releaseNext = true;
				}
			}
		}
		GJBaseGameLayer::update(dt);
	}
};

class $modify(FFBPlayLayer, PlayLayer) {
	void resetLevel() {
		g_pending[0] = Pending{};
		g_pending[1] = Pending{};
		PlayLayer::resetLevel();
	}
};
