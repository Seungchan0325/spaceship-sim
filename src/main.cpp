#include "imgui.h"
#include "imgui-SFML.h"

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <cmath>
#include <deque>
#include <vector>

class Body
{
public:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Vector2f force;
    float mass;
    float radius;
    sf::Color color;

    std::deque<sf::Vector2f> trail;
    std::size_t trailMax = 40000;
};

static sf::Vector2f gravAccel(const Body& a, const Body& b, float G) {
    sf::Vector2f d = b.position - a.position;
    float r2 = d.x*d.x + d.y*d.y;
    float invR = 1.f / std::sqrt(r2);
    float invR3 = invR * invR * invR;
    return (G * b.mass) * d * invR3;
}

float sampleAccum = 0.f;
float sampleEvery = 6.f * 60.f * 60.f;

bool paused = true;
bool drawOrbit = true;
float timeScale = 1.0f;
float curTime = 0;
float G = 6.67430e-20;
float dtFixed = 60.f;
// float dtFixed = 0.008f;
int substeps = 1;
std::vector<Body> bodies;

void PhysicsInit()
{
    // Solar system
    Body sun;
    sun.position = sf::Vector2f(0, 0);
    sun.velocity = sf::Vector2f(0, 0);
    sun.force = sf::Vector2f(0, 0);
    sun.mass = 1.989E30;
    sun.radius = 695500;
    sun.color = sf::Color(255, 215, 0, 255);
    bodies.push_back(sun);

    Body mercury;
    mercury.position = sf::Vector2f(57.9e6, 0);
    mercury.velocity = sf::Vector2f(0, 47.36);
    mercury.force = sf::Vector2f(0, 0);
    mercury.mass = 3.3022E23;
    mercury.radius = 2439.7;
    mercury.color = sf::Color(190, 190, 190, 255);
    bodies.push_back(mercury);

    Body venus;
    venus.position = sf::Vector2f(108.2e6, 0);
    venus.velocity = sf::Vector2f(0, 35.02);
    venus.force = sf::Vector2f(0, 0);
    venus.mass = 4.8690E24;
    venus.radius = 6051.8;
    venus.color = sf::Color(238, 220, 130, 255);
    bodies.push_back(venus);

    Body earth;
    earth.position = sf::Vector2f(149.6e6, 0);
    earth.velocity = sf::Vector2f(0, 29.78);
    earth.force = sf::Vector2f(0, 0);
    earth.mass = 5.9742E24;
    earth.radius = 6378.14;
    earth.color = sf::Color(30, 144, 255, 255);
    bodies.push_back(earth);

    Body moon;
    moon.position = sf::Vector2f(149.6e6 + 0.3844e6, 0);
    moon.velocity = sf::Vector2f(0, 29.78 + 1.022);
    moon.force = sf::Vector2f(0, 0);
    moon.mass = 7.35E22;
    moon.radius = 1738.2;
    moon.color = sf::Color(180, 180, 180, 255);
    bodies.push_back(moon);

    Body mars;
    mars.position = sf::Vector2f(227.9e6, 0);
    mars.velocity = sf::Vector2f(0, 24.07);
    mars.force = sf::Vector2f(0, 0);
    mars.mass = 6.4191E23;
    mars.radius = 3396.2;
    mars.color = sf::Color(178, 34, 34, 255);
    bodies.push_back(mars);

    Body jupiter;
    jupiter.position = sf::Vector2f(778.5e6, 0);
    jupiter.velocity = sf::Vector2f(0, 13.07);
    jupiter.force = sf::Vector2f(0, 0);
    jupiter.mass = 1.8988E27;
    jupiter.radius = 71492;
    jupiter.color = sf::Color(210, 180, 140, 255);
    bodies.push_back(jupiter);

    Body saturn;
    saturn.position = sf::Vector2f(1434e6, 0);
    saturn.velocity = sf::Vector2f(0, 9.68);
    saturn.force = sf::Vector2f(0, 0);
    saturn.mass = 5.6852E26;
    saturn.radius = 60268;
    saturn.color = sf::Color(240, 230, 140, 255);
    bodies.push_back(saturn);

    Body uranus;
    uranus.position = sf::Vector2f(2871e6, 0);
    uranus.velocity = sf::Vector2f(0, 6.80);
    uranus.force = sf::Vector2f(0, 0);
    uranus.mass = 8.6840E25;
    uranus.radius = 25559;
    uranus.color = sf::Color(102, 205, 170, 255);
    bodies.push_back(uranus);

    Body neptune;
    neptune.position = sf::Vector2f(4495e6, 0);
    neptune.velocity = sf::Vector2f(0, 5.43);
    neptune.force = sf::Vector2f(0, 0);
    neptune.mass = 1.0245E26;
    neptune.radius = 24764;
    neptune.color = sf::Color(0, 0, 205, 255);
    bodies.push_back(neptune);

    Body pluto;
    pluto.position = sf::Vector2f(5906e6, 0);
    pluto.velocity = sf::Vector2f(0, 4.74);
    pluto.force = sf::Vector2f(0, 0);
    pluto.mass = 1.3E22;
    pluto.radius = 1195;
    pluto.color = sf::Color(194, 178, 128, 255);
    bodies.push_back(pluto);
}


int main() {
    const sf::Vector2u winSize{1280, 720};
    sf::RenderWindow window(sf::VideoMode(winSize), "SPACESHIP SIMULATOR");
    window.setFramerateLimit(120);

    ImGui::SFML::Init(window);

    sf::View view({0.f, 0.f}, {1280000.0f, 720000.0f});
    bool dragging = false;
    sf::Vector2f dragStartWorld{};
    sf::Vector2i dragStartMouse{};

    PhysicsInit();

    sf::Clock deltaClock;
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (auto* we = event->getIf<sf::Event::MouseWheelScrolled>()) {
                auto before = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
                float factor = (we->delta > 0) ? 0.9f : 1.1f;
                view.zoom(factor);
                auto after = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
                view.move(before - after);
            }

            if (auto* bp = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (bp->button == sf::Mouse::Button::Right) {
                    dragging = true;
                    dragStartMouse = sf::Mouse::getPosition(window);
                    dragStartWorld = window.mapPixelToCoords(dragStartMouse, view);
                }
            }
            if (auto* br = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (br->button == sf::Mouse::Button::Right) dragging = false;
            }

            if (auto* mm = event->getIf<sf::Event::MouseMoved>()) {
                if (dragging) {
                    sf::Vector2i curMouse{mm->position.x, mm->position.y};
                    auto curWorld = window.mapPixelToCoords(curMouse, view);
                    view.move(dragStartWorld - curWorld);
                }
            }

            if (event->is<sf::Event::Resized>()) {
                auto size = event->getIf<sf::Event::Resized>()->size;
                const float worldHeight = view.getSize().y;
                float aspect = static_cast<float>(size.x) / size.y;
                view.setSize({worldHeight * aspect, worldHeight});
                window.setView(view);
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("SPACESHIP SIM");
        ImGui::Checkbox("Paused", &paused);
        ImGui::SliderFloat("Time scale", &timeScale, 0.0f, 100.0f, "%.2f");
        ImGui::SliderInt("Substeps", &substeps, 1, 100);
        ImGui::Checkbox("Orbit", &drawOrbit);
        ImGui::Text("%f days", curTime / 60.0 / 60.0 / 24.0);
        ImGui::End();

        if (!paused && timeScale > 0.f) {
            float step = dtFixed * timeScale / static_cast<float>(substeps);
            curTime += dtFixed * timeScale;
            for (int s = 0; s < substeps; ++s) {
                std::vector<sf::Vector2f> acc(bodies.size(), {0.f, 0.f});
                for (size_t i = 0; i < bodies.size(); ++i) {
                    for (size_t j = i + 1; j < bodies.size(); ++j) {
                        auto aij = gravAccel(bodies[i], bodies[j], G);
                        auto aji = gravAccel(bodies[j], bodies[i], G);
                        acc[i] += aij;
                        acc[j] += aji;
                    }
                }
                for (size_t i = 0; i < bodies.size(); ++i) {
                    bodies[i].velocity = (bodies[i].velocity + acc[i] * step);
                    bodies[i].position += bodies[i].velocity * step;
                }
            }

            sampleAccum += dtFixed * timeScale;
            if(drawOrbit && sampleAccum >= sampleEvery) {
                sampleAccum = 0.f;
                for(auto& b : bodies) {
                    if(b.trail.empty() || b.trail.back() != b.position) {
                        b.trail.push_back(b.position);
                        if(b.trail.size() > b.trailMax) b.trail.pop_front();
                    }
                }
            }
        }

        // 지구를 중심으로 설정
        // view.setCenter(bodies[3].position);

        window.clear(sf::Color::Black);
        window.setView(view);

        if(drawOrbit) {
            for(const auto& b : bodies) {
                if (b.trail.size() < 2) continue;

                sf::VertexArray strip(sf::PrimitiveType::LineStrip, b.trail.size());
                const std::size_t n = b.trail.size();
                for (std::size_t i = 0; i < n; ++i) {
                    strip[i].position = b.trail[i];

                    strip[i].color = sf::Color(b.color.r, b.color.g, b.color.b, 255);
                }
                window.draw(strip);
            }
        }

        for (auto& b : bodies) {
            sf::CircleShape c(b.radius);
            c.setOrigin({b.radius, b.radius});
            c.setPosition(b.position);
            c.setFillColor(b.color);
            window.draw(c);
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}