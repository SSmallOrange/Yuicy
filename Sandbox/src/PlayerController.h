#pragma once

#include "Yuicy.h"
#include <box2d/b2_body.h>

// 玩家控制器脚本示例
class PlayerController : public Yuicy::ScriptableEntity
{
public:
	// 可配置参数
	float MoveSpeed = 5.0f;
	float JumpForce = 10.0f;

private:
	bool m_IsGrounded = false;
	int m_GroundContactCount = 0;  // 接触地面的碰撞体数量

public:
	void OnCreate() override
	{
		YUICY_INFO("PlayerController: OnCreate called!");
	}

	void OnDestroy() override
	{
		YUICY_INFO("PlayerController: OnDestroy called!");
	}

	void OnUpdate(Yuicy::Timestep ts) override
	{
		// 获取刚体
		if (!HasComponent<Yuicy::Rigidbody2DComponent>())
			return;

		auto& rb = GetComponent<Yuicy::Rigidbody2DComponent>();
		b2Body* body = static_cast<b2Body*>(rb.RuntimeBody);
		if (!body)
			return;

		// 获取当前速度
		b2Vec2 velocity = body->GetLinearVelocity();

		// ==================== 水平移动 ====================
		float moveInput = 0.0f;

		if (Yuicy::Input::IsKeyPressed(Yuicy::Key::A) ||
			Yuicy::Input::IsKeyPressed(Yuicy::Key::Left))
		{
			moveInput -= 1.0f;
		}

		if (Yuicy::Input::IsKeyPressed(Yuicy::Key::D) ||
			Yuicy::Input::IsKeyPressed(Yuicy::Key::Right))
		{
			moveInput += 1.0f;
		}

		// 设置水平速度
		velocity.x = moveInput * MoveSpeed;

		// ==================== 跳跃 ====================
		if (Yuicy::Input::IsKeyPressed(Yuicy::Key::Space) ||
			Yuicy::Input::IsKeyPressed(Yuicy::Key::W) ||
			Yuicy::Input::IsKeyPressed(Yuicy::Key::Up))
		{
			if (m_IsGrounded)
			{
				velocity.y = JumpForce;
				m_IsGrounded = false;
			}
		}

		// 应用速度
		body->SetLinearVelocity(velocity);

		// ==================== 动画切换 ====================
		if (HasComponent<Yuicy::AnimationComponent>())
		{
			auto& anim = GetComponent<Yuicy::AnimationComponent>();

			// 根据状态切换动画
			if (!m_IsGrounded)
			{
				// 空中状态
				anim.Play("Jump");
			}
			else if (moveInput != 0.0f)
			{
				// 地面移动
				anim.Play("Walk");
			}
			else
			{
				// 地面静止
				anim.Play("Idle");
			}
		}

		// ==================== 精灵翻转 ====================
		if (HasComponent<Yuicy::SpriteRendererComponent>() && moveInput != 0.0f)
		{
			auto& sprite = GetComponent<Yuicy::SpriteRendererComponent>();
			sprite.FlipX = (moveInput < 0.0f);
		}
	}

	void OnCollisionEnter(Yuicy::Entity other) override
	{
		// 检测是否落地
		if (other.HasComponent<Yuicy::TagComponent>())
		{
			auto& tag = other.GetComponent<Yuicy::TagComponent>();
			if (tag.Tag == "Ground" || tag.Tag == "Platform")
			{
				m_GroundContactCount++;
				m_IsGrounded = true;
				YUICY_INFO("Player landed on: {}", tag.Tag);
			}
		}
	}

	void OnCollisionExit(Yuicy::Entity other) override
	{
		if (other.HasComponent<Yuicy::TagComponent>())
		{
			auto& tag = other.GetComponent<Yuicy::TagComponent>();
			if (tag.Tag == "Ground" || tag.Tag == "Platform")
			{
				m_GroundContactCount--;
				if (m_GroundContactCount <= 0)
				{
					m_GroundContactCount = 0;
					m_IsGrounded = false;
				}
			}
		}
	}

	void OnTriggerEnter(Yuicy::Entity other) override
	{
		if (other.HasComponent<Yuicy::TagComponent>())
		{
			auto& tag = other.GetComponent<Yuicy::TagComponent>();
			YUICY_INFO("Player entered trigger: {}", tag.Tag);

			// 示例：收集金币
			// if (tag.Tag == "Coin")
			// {
			//     // 增加分数
			//     // 销毁金币
			// }
		}
	}
};