#include <Yuicy.h>
#include <imgui/imgui.h>

#include "UILayer.h"

namespace TinyDungeon {

	UILayer::UILayer()
		: Layer("UILayer")
	{
	}

	void UILayer::OnAttach()
	{
		LoadTextures();
	}

	void UILayer::OnDetach()
	{
		m_completeHeart.reset();
		m_halfHeart.reset();
		m_emptyHeart.reset();
		for (int i = 0; i < 10; i++)
			m_numberTextures[i].reset();
	}

	void UILayer::LoadTextures()
	{
		m_completeHeart = Yuicy::Texture2D::Create("assets/textures/heart/complete_heart.png");
		m_halfHeart = Yuicy::Texture2D::Create("assets/textures/heart/half_heart.png");
		m_emptyHeart = Yuicy::Texture2D::Create("assets/textures/heart/zero_heart.png");

		for (int i = 0; i < 10; i++)
		{
			std::string path = "assets/textures/number/" + std::to_string(i) + ".png";
			m_numberTextures[i] = Yuicy::Texture2D::Create(path);
		}
	}

	void UILayer::OnUpdate(Yuicy::Timestep ts)
	{
	}

	void UILayer::OnImGuiRender()
	{
		RenderHealth();
		RenderScore();
	}

	void UILayer::OnEvent(Yuicy::Event& e)
	{
	}

	void UILayer::SetHealth(int health)
	{
		m_health = std::clamp(health, 0, 6);
	}

	void UILayer::SetScore(int score)
	{
		m_score = std::clamp(score, 0, 99);
	}

	void UILayer::RenderHealth()
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGuiWindowFlags flags = 
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoNav;

		ImGui::SetNextWindowViewport(viewport->ID);

		float windowWidth = m_heartSize * 3 + m_heartSpacing * 2 + 20.0f;
		ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 10.0f, viewport->Pos.y + m_padding), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(windowWidth, m_heartSize + 10.0f), ImGuiCond_Always);

		ImGui::Begin("##Health", nullptr, flags);

		for (int i = 0; i < 3; i++)
		{
			int heartHP = m_health - i * 2;
			Yuicy::Ref<Yuicy::Texture2D> tex;

			if (heartHP >= 2)
				tex = m_completeHeart;
			else if (heartHP == 1)
				tex = m_halfHeart;
			else
				tex = m_emptyHeart;

			if (tex)
			{
				ImGui::Image((ImTextureID)(uintptr_t)tex->GetRendererID(),
					ImVec2(m_heartSize, m_heartSize),
					ImVec2(0, 1), ImVec2(1, 0));
			}

			if (i < 2)
				ImGui::SameLine(0, m_heartSpacing);
		}

		ImGui::End();
	}

	void UILayer::RenderScore()
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGuiWindowFlags flags = 
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoNav;

		ImGui::SetNextWindowViewport(viewport->ID);

		float windowWidth = m_numberSize * 2 + 20.0f;
		float posX = viewport->Pos.x + viewport->Size.x - windowWidth - 10.0f;
		ImGui::SetNextWindowPos(ImVec2(posX, viewport->Pos.y + m_padding), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(windowWidth, m_numberSize + 10.0f), ImGuiCond_Always);

		ImGui::Begin("##Score", nullptr, flags);

		int tens = (m_score / 10) % 10;
		int ones = m_score % 10;

		if (m_numberTextures[tens])
		{
			ImGui::Image((ImTextureID)(uintptr_t)m_numberTextures[tens]->GetRendererID(),
				ImVec2(m_numberSize, m_numberSize),
				ImVec2(0, 1), ImVec2(1, 0));
		}

		ImGui::SameLine(0, 0);

		if (m_numberTextures[ones])
		{
			ImGui::Image((ImTextureID)(uintptr_t)m_numberTextures[ones]->GetRendererID(),
				ImVec2(m_numberSize, m_numberSize),
				ImVec2(0, 1), ImVec2(1, 0));
		}

		ImGui::End();
	}

}
