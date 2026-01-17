#pragma once

#include <Yuicy.h>

namespace TinyDungeon {

	class UILayer : public Yuicy::Layer
	{
	public:
		UILayer();
		~UILayer() override = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(Yuicy::Timestep ts) override;
		void OnImGuiRender() override;
		void OnEvent(Yuicy::Event& e) override;

		void SetHealth(int health);  // 0-6
		void SetScore(int score);    // 0-99

		int GetHealth() const { return m_health; }
		int GetScore() const { return m_score; }

	private:
		void LoadTextures();
		void RenderHealth();
		void RenderScore();

	private:
		int m_health = 6;
		int m_score = 0;

		Yuicy::Ref<Yuicy::Texture2D> m_completeHeart;
		Yuicy::Ref<Yuicy::Texture2D> m_halfHeart;
		Yuicy::Ref<Yuicy::Texture2D> m_emptyHeart;

		Yuicy::Ref<Yuicy::Texture2D> m_numberTextures[10];

		float m_heartSize = 32.0f;
		float m_heartSpacing = 4.0f;
		float m_numberSize = 24.0f;
		float m_padding = 48.0f;
	};

}
