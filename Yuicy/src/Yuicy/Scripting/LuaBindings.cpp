#include "pch.h"
#include "LuaBindings.h"
#include "Yuicy/Core/Log.h"
#include "Yuicy/Core/Input.h"
#include "Yuicy/Core/KeyCodes.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Components.h"
#include "Yuicy/Scene/Scene.h"

#include <box2d/b2_body.h>
#include <glm/glm.hpp>

namespace Yuicy {

	namespace LuaBindings
	{
		void RegisterAll(sol::state& lua)
		{
			RegisterLog(lua);
			RegisterMath(lua);
			RegisterInput(lua);
			RegisterComponents(lua);
			RegisterEntity(lua);
			RegisterScene(lua);
		}

		void RegisterLog(sol::state& lua)
		{
			// Simple print function that uses engine logging
			lua.set_function("print", [](const std::string& msg) {
				YUICY_CORE_INFO("[Lua] {}", msg);
			});

			lua.set_function("Log", sol::overload(
				[](const std::string& msg) { YUICY_CORE_INFO("[Lua] {}", msg); },
				[](const std::string& msg, int level) {
					switch (level)
					{
					case 0: YUICY_CORE_TRACE("[Lua] {}", msg); break;
					case 1: YUICY_CORE_INFO("[Lua] {}", msg); break;
					case 2: YUICY_CORE_WARN("[Lua] {}", msg); break;
					case 3: YUICY_CORE_ERROR("[Lua] {}", msg); break;
					default: YUICY_CORE_INFO("[Lua] {}", msg); break;
					}
				}
			));
		}

		void RegisterMath(sol::state& lua)
		{
			// glm::vec2
			lua.new_usertype<glm::vec2>("Vec2",
				sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
				"x", &glm::vec2::x,
				"y", &glm::vec2::y,
				sol::meta_function::addition, [](const glm::vec2& a, const glm::vec2& b) { return a + b; },
				sol::meta_function::subtraction, [](const glm::vec2& a, const glm::vec2& b) { return a - b; },
				sol::meta_function::multiplication, sol::overload(
					[](const glm::vec2& a, float b) { return a * b; },
					[](float a, const glm::vec2& b) { return a * b; }
				)
			);

			// glm::vec3
			lua.new_usertype<glm::vec3>("Vec3",
				sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
				"x", &glm::vec3::x,
				"y", &glm::vec3::y,
				"z", &glm::vec3::z,
				sol::meta_function::addition, [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
				sol::meta_function::subtraction, [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
				sol::meta_function::multiplication, sol::overload(
					[](const glm::vec3& a, float b) { return a * b; },
					[](float a, const glm::vec3& b) { return a * b; }
				)
			);

			// glm::vec4
			lua.new_usertype<glm::vec4>("Vec4",
				sol::constructors<glm::vec4(), glm::vec4(float), glm::vec4(float, float, float, float)>(),
				"x", &glm::vec4::x,
				"y", &glm::vec4::y,
				"z", &glm::vec4::z,
				"w", &glm::vec4::w
			);
		}

		void RegisterInput(sol::state& lua)
		{
			// Input static functions
			lua.new_usertype<Input>("Input",
				sol::no_constructor,
				"IsKeyPressed", [](KeyCode key) { return Input::IsKeyPressed(key); },
				"IsMouseButtonPressed", [](MouseCode button) { return Input::IsMouseButtonPressed(button); },
				"GetMousePosition", []() {
					auto [x, y] = Input::GetMousePosition();
					return std::make_tuple(x, y);
				},
				"GetMouseX", &Input::GetMouseX,
				"GetMouseY", &Input::GetMouseY
			);

			// Key codes as a table
			sol::table keyTable = lua.create_named_table("Key");
			keyTable["Space"] = Key::Space;
			keyTable["Escape"] = Key::Escape;
			keyTable["Enter"] = Key::Enter;
			keyTable["Tab"] = Key::Tab;
			keyTable["Backspace"] = Key::Backspace;

			// Arrow keys
			keyTable["Right"] = Key::Right;
			keyTable["Left"] = Key::Left;
			keyTable["Down"] = Key::Down;
			keyTable["Up"] = Key::Up;

			// Letters
			keyTable["A"] = Key::A;
			keyTable["B"] = Key::B;
			keyTable["C"] = Key::C;
			keyTable["D"] = Key::D;
			keyTable["E"] = Key::E;
			keyTable["F"] = Key::F;
			keyTable["G"] = Key::G;
			keyTable["H"] = Key::H;
			keyTable["I"] = Key::I;
			keyTable["J"] = Key::J;
			keyTable["K"] = Key::K;
			keyTable["L"] = Key::L;
			keyTable["M"] = Key::M;
			keyTable["N"] = Key::N;
			keyTable["O"] = Key::O;
			keyTable["P"] = Key::P;
			keyTable["Q"] = Key::Q;
			keyTable["R"] = Key::R;
			keyTable["S"] = Key::S;
			keyTable["T"] = Key::T;
			keyTable["U"] = Key::U;
			keyTable["V"] = Key::V;
			keyTable["W"] = Key::W;
			keyTable["X"] = Key::X;
			keyTable["Y"] = Key::Y;
			keyTable["Z"] = Key::Z;

			// Numbers
			keyTable["D0"] = Key::D0;
			keyTable["D1"] = Key::D1;
			keyTable["D2"] = Key::D2;
			keyTable["D3"] = Key::D3;
			keyTable["D4"] = Key::D4;
			keyTable["D5"] = Key::D5;
			keyTable["D6"] = Key::D6;
			keyTable["D7"] = Key::D7;
			keyTable["D8"] = Key::D8;
			keyTable["D9"] = Key::D9;

			// Function keys
			keyTable["F1"] = Key::F1;
			keyTable["F2"] = Key::F2;
			keyTable["F3"] = Key::F3;
			keyTable["F4"] = Key::F4;
			keyTable["F5"] = Key::F5;
			keyTable["F6"] = Key::F6;
			keyTable["F7"] = Key::F7;
			keyTable["F8"] = Key::F8;
			keyTable["F9"] = Key::F9;
			keyTable["F10"] = Key::F10;
			keyTable["F11"] = Key::F11;
			keyTable["F12"] = Key::F12;

			// Modifiers
			keyTable["LeftShift"] = Key::LeftShift;
			keyTable["LeftControl"] = Key::LeftControl;
			keyTable["LeftAlt"] = Key::LeftAlt;
			keyTable["RightShift"] = Key::RightShift;
			keyTable["RightControl"] = Key::RightControl;
			keyTable["RightAlt"] = Key::RightAlt;
		}

		void RegisterComponents(sol::state& lua)
		{
			// TransformComponent
			lua.new_usertype<TransformComponent>("TransformComponent",
				sol::no_constructor,
				"Translation", &TransformComponent::Translation,
				"Rotation", &TransformComponent::Rotation,
				"Scale", &TransformComponent::Scale,
				"GetTransform", &TransformComponent::GetTransform
			);

			// SpriteRendererComponent
			lua.new_usertype<SpriteRendererComponent>("SpriteRendererComponent",
				sol::no_constructor,
				"Color", &SpriteRendererComponent::Color,
				"TilingFactor", &SpriteRendererComponent::TilingFactor,
				"FlipX", &SpriteRendererComponent::FlipX,
				"FlipY", &SpriteRendererComponent::FlipY,
				"SortingOrder", &SpriteRendererComponent::SortingOrder
			);

			// TagComponent
			lua.new_usertype<TagComponent>("TagComponent",
				sol::no_constructor,
				"Tag", &TagComponent::Tag
			);

			// Rigidbody2DComponent
			lua.new_usertype<Rigidbody2DComponent>("Rigidbody2DComponent",
				sol::no_constructor,
				"SetLinearVelocity", [](Rigidbody2DComponent& rb, float vx, float vy) {
					if (rb.RuntimeBody)
					{
						b2Body* body = static_cast<b2Body*>(rb.RuntimeBody);
						body->SetLinearVelocity(b2Vec2(vx, vy));
					}
				},
				"GetLinearVelocity", [](Rigidbody2DComponent& rb) -> glm::vec2 {
					if (rb.RuntimeBody)
					{
						b2Body* body = static_cast<b2Body*>(rb.RuntimeBody);
						b2Vec2 vel = body->GetLinearVelocity();
						return { vel.x, vel.y };
					}
					return { 0.0f, 0.0f };
				},
				"SetGravityScale", [](Rigidbody2DComponent& rb, float scale) {
					if (rb.RuntimeBody)
					{
						b2Body* body = static_cast<b2Body*>(rb.RuntimeBody);
						body->SetGravityScale(scale);
					}
				}
			);

			// AnimationComponent
			lua.new_usertype<AnimationComponent>("AnimationComponent",
				sol::no_constructor,
				"Play", sol::overload(
					[](AnimationComponent& anim, const std::string& clipName) { anim.Play(clipName); },
					[](AnimationComponent& anim, const std::string& clipName, bool forceRestart) { anim.Play(clipName, forceRestart); }
				),
				"Stop", &AnimationComponent::Stop,
				"Pause", &AnimationComponent::Pause,
				"Resume", &AnimationComponent::Resume,
				"IsPlaying", sol::overload(
					sol::resolve<bool() const>(&AnimationComponent::IsPlaying),
					sol::resolve<bool(const std::string&) const>(&AnimationComponent::IsPlaying)
				),
				"IsFinished", &AnimationComponent::IsFinished,
				"GetCurrentClipName", [](AnimationComponent& anim) { return anim.State.CurrentClipName; }
			);

			// CameraComponent
			lua.new_usertype<CameraComponent>("CameraComponent",
				sol::no_constructor,
				"Primary", &CameraComponent::Primary,
				"FixedAspectRatio", &CameraComponent::FixedAspectRatio,
				"GetOrthographicSize", [](CameraComponent& cc) -> float {
					return cc.Camera.GetOrthographicSize();
				},
				"SetOrthographicSize", [](CameraComponent& cc, float size) {
					cc.Camera.SetOrthographicSize(size);
				}
			);

			// ProjectileComponent
			lua.new_usertype<ProjectileComponent>("ProjectileComponent",
				sol::no_constructor,
				"direction", &ProjectileComponent::direction,
				"speed", &ProjectileComponent::speed,
				"lifetime", &ProjectileComponent::lifetime,
				"damage", &ProjectileComponent::damage,
				"destroyOnHit", &ProjectileComponent::destroyOnHit,
				"elapsedTime", &ProjectileComponent::elapsedTime
			);
		}

		void RegisterEntity(sol::state& lua)
		{
			lua.new_usertype<Entity>("Entity",
				sol::no_constructor,
				"GetTransform", [](Entity& e) -> TransformComponent& {
					return e.GetComponent<TransformComponent>();
				},
				"HasTransform", [](Entity& e) -> bool {
					return e.HasComponent<TransformComponent>();
				},
				"GetSprite", [](Entity& e) -> SpriteRendererComponent& {
					return e.GetComponent<SpriteRendererComponent>();
				},
				"GetTag", [](Entity& e) -> std::string {
					return e.GetComponent<TagComponent>().Tag;
				},
				"HasSprite", [](Entity& e) -> bool {
					return e.HasComponent<SpriteRendererComponent>();
				},
				"GetAnimation", [](Entity& e) -> AnimationComponent& {
					return e.GetComponent<AnimationComponent>();
				},
				"HasAnimation", [](Entity& e) -> bool {
					return e.HasComponent<AnimationComponent>();
				},
				"GetRigidbody", [](Entity& e) -> Rigidbody2DComponent& {
					return e.GetComponent<Rigidbody2DComponent>();
				},
				"HasRigidbody", [](Entity& e) -> bool {
					return e.HasComponent<Rigidbody2DComponent>();
				},
				"GetCamera", [](Entity& e) -> CameraComponent& {
					return e.GetComponent<CameraComponent>();
				},
				"HasCamera", [](Entity& e) -> bool {
					return e.HasComponent<CameraComponent>();
				},
				"GetProjectile", [](Entity& e) -> ProjectileComponent& {
					return e.GetComponent<ProjectileComponent>();
				},
				"HasProjectile", [](Entity& e) -> bool {
					return e.HasComponent<ProjectileComponent>();
				},
				"IsValid", [](Entity& e) -> bool {
					return (bool)e;
				}
			);
		}

	void RegisterScene(sol::state& lua)
		{
			// Scene usertype
			lua.new_usertype<Scene>("Scene",
				sol::no_constructor,
				"FindEntityByName", &Scene::FindEntityByName,
				"CreateProjectile", &Scene::CreateProjectile
			);

			// Global Scene table with static-like functions
			// These use the entity's scene reference
			sol::table sceneTable = lua.create_named_table("Scene");
			sceneTable.set_function("FindEntityByName", [](Entity& self, const std::string& name) -> Entity {
				if (!self)
					return Entity{};
				// Get scene from entity and find
				Scene* scene = self.GetScene();
				if (scene)
					return scene->FindEntityByName(name);
				return Entity{};
			});

			// CreateProjectile from Lua (with optional config parameters)
			sceneTable.set_function("CreateProjectile", [](Entity& self, float x, float y, float dirX, float dirY, 
				sol::optional<float> speed, sol::optional<float> lifetime, sol::optional<float> sizeX, sol::optional<float> sizeY,
				sol::optional<float> r, sol::optional<float> g, sol::optional<float> b) -> Entity {
				if (!self)
					return Entity{};
				Scene* scene = self.GetScene();
				if (scene)
				{
					ProjectileConfig config;
					config.speed = speed.value_or(15.0f);
					config.lifetime = lifetime.value_or(3.0f);
					config.size = { sizeX.value_or(0.2f), sizeY.value_or(0.2f) };
					if (r && g && b)
						config.color = { r.value(), g.value(), b.value(), 1.0f };
					return scene->CreateProjectile({ x, y }, { dirX, dirY }, config);
				}
				return Entity{};
			});
		}
	}

}
