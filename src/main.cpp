#include "imgui.h"
#include "imgui-SFML.h"

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <cmath>
#include <deque>
#include <vector>

using Vector2d = sf::Vector2<double>;

const int alphabet = 256;

class Body;

class TrieNode {
public:
    Body* element;
    TrieNode *ch[alphabet];
    TrieNode() : element(nullptr), ch({nullptr}) {};
    ~TrieNode() {
        for(int i =0; i < alphabet; i++) if(ch[i]) delete ch[i];
    }
    void insert(const char* s, Body* e)
    {
        if(*s == '\0') {
            element = e;
            return;
        }
        if(ch[*s] == nullptr) ch[*s] = new TrieNode;
        ch[*s]->insert(s+1, e);
    }
    Body* find(const char* s)
    {
        if(*s == '\0') return element;
        if(ch[*s] == nullptr) return nullptr;
        return ch[*s]->find(s+1);
    }
};

class Body
{
public:
    std::string name;

    Vector2d position;
    Vector2d velocity;
    Vector2d force;
    double mass;
    double radius;
    sf::Color color;

    std::deque<Vector2d> trail;
    std::size_t trailMax = 40000;
};

static Vector2d gravAccel(const Body& a, const Body& b, double G) {
    Vector2d d = b.position - a.position;
    double r2 = d.x*d.x + d.y*d.y;
    double invR = 1.f / std::sqrt(r2);
    double invR3 = invR * invR * invR;
    return (G * b.mass) * d * invR3;
}

Vector2d spaceship_force = {0.f, 0.f};

bool paused = true;
bool drawOrbit = true;
double sampleAccum = 0.f;
double sampleEvery = 6.f * 60.f * 60.f;
float timeScale = 1.0f;
double curTime = 0;
double G = 6.67430e-20;
double dtFixed = 60.f;
// double dtFixed = 0.008f;
int substeps = 1;
std::vector<Body*> bodies;

Body sun;
Body mercury;
Body venus;
Body earth;
Body moon;
Body mars;
Body jupiter;
Body saturn;
Body uranus;
Body neptune;
Body pluto;
Body spaceship;

void PhysicsInit()
{
    // Solar system
    sun.name = "sun";
    sun.position = Vector2d(0, 0);
    sun.velocity = Vector2d(0, 0);
    sun.force = Vector2d(0, 0);
    sun.mass = 1.989E30;
    sun.radius = 695500;
    sun.color = sf::Color(255, 215, 0, 255);
    bodies.push_back(&sun);

    mercury.name = "mercury";
    mercury.position = Vector2d(57.9e6, 0);
    mercury.velocity = Vector2d(0, 47.36);
    mercury.force = Vector2d(0, 0);
    mercury.mass = 3.3022E23;
    mercury.radius = 2439.7;
    mercury.color = sf::Color(190, 190, 190, 255);
    bodies.push_back(&mercury);

    venus.name = "venus";
    venus.position = Vector2d(108.2e6, 0);
    venus.velocity = Vector2d(0, 35.02);
    venus.force = Vector2d(0, 0);
    venus.mass = 4.8690E24;
    venus.radius = 6051.8;
    venus.color = sf::Color(238, 220, 130, 255);
    bodies.push_back(&venus);

    earth.name = "earth";
    earth.position = Vector2d(149.6e6, 0);
    earth.velocity = Vector2d(0, 29.78);
    earth.force = Vector2d(0, 0);
    earth.mass = 5.9742E24;
    earth.radius = 6378.14;
    earth.color = sf::Color(30, 144, 255, 255);
    bodies.push_back(&earth);

    moon.name = "moon";
    moon.position = Vector2d(149.6e6 + 0.3844e6, 0);
    moon.velocity = Vector2d(0, 29.78 + 1.022);
    moon.force = Vector2d(0, 0);
    moon.mass = 7.35E22;
    moon.radius = 1738.2;
    moon.color = sf::Color(180, 180, 180, 255);
    bodies.push_back(&moon);

    mars.name = "mars";
    mars.position = Vector2d(227.9e6, 0);
    mars.velocity = Vector2d(0, 24.07);
    mars.force = Vector2d(0, 0);
    mars.mass = 6.4191E23;
    mars.radius = 3396.2;
    mars.color = sf::Color(178, 34, 34, 255);
    bodies.push_back(&mars);

    jupiter.name = "jupiter";
    jupiter.position = Vector2d(778.5e6, 0);
    jupiter.velocity = Vector2d(0, 13.07);
    jupiter.force = Vector2d(0, 0);
    jupiter.mass = 1.8988E27;
    jupiter.radius = 71492;
    jupiter.color = sf::Color(210, 180, 140, 255);
    bodies.push_back(&jupiter);

    saturn.name = "saturn";
    saturn.position = Vector2d(1434e6, 0);
    saturn.velocity = Vector2d(0, 9.68);
    saturn.force = Vector2d(0, 0);
    saturn.mass = 5.6852E26;
    saturn.radius = 60268;
    saturn.color = sf::Color(240, 230, 140, 255);
    bodies.push_back(&saturn);

    uranus.name = "uranus";
    uranus.position = Vector2d(2871e6, 0);
    uranus.velocity = Vector2d(0, 6.80);
    uranus.force = Vector2d(0, 0);
    uranus.mass = 8.6840E25;
    uranus.radius = 25559;
    uranus.color = sf::Color(102, 205, 170, 255);
    bodies.push_back(&uranus);

    neptune.name = "neptune";
    neptune.position = Vector2d(4495e6, 0);
    neptune.velocity = Vector2d(0, 5.43);
    neptune.force = Vector2d(0, 0);
    neptune.mass = 1.0245E26;
    neptune.radius = 24764;
    neptune.color = sf::Color(0, 0, 205, 255);
    bodies.push_back(&neptune);

    pluto.name = "pluto";
    pluto.position = Vector2d(5906e6, 0);
    pluto.velocity = Vector2d(0, 4.74);
    pluto.force = Vector2d(0, 0);
    pluto.mass = 1.3E22;
    pluto.radius = 1195;
    pluto.color = sf::Color(194, 178, 128, 255);
    bodies.push_back(&pluto);

    spaceship.name = "spaceship";
    spaceship.position = Vector2d(earth.position.x + earth.radius + 100, 0);
    spaceship.velocity = Vector2d(0, 11.2);
    spaceship.force = Vector2d(0, 0);
    spaceship.mass = 1000;
    spaceship.radius = 0.001;
    spaceship.color = sf::Color::Green;
    bodies.push_back(&spaceship);
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

    int currentView = 0;
    std::vector<std::string> viewItems = {"Free"};
    TrieNode* root = new TrieNode;
    for(const auto& b : bodies) {
        viewItems.push_back(b->name);
        root->insert(b->name.c_str(), b);
    }

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
        if(ImGui::BeginCombo("View", viewItems[currentView].c_str())) {
            for(int i = 0; i < viewItems.size(); i++) {
                bool is_selected = (currentView == i);
                if(ImGui::Selectable(viewItems[i].c_str(), is_selected))
                    currentView = i;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::Text("%f days", curTime / 60.0 / 60.0 / 24.0);
        ImGui::End();

        if (!paused && timeScale > 0.f) {
            double step = dtFixed * timeScale / static_cast<double>(substeps);
            curTime += dtFixed * timeScale;
            for (int s = 0; s < substeps; ++s) {
                std::vector<Vector2d> acc(bodies.size(), {0.f, 0.f});
                for (size_t i = 0; i < bodies.size(); ++i) {
                    for (size_t j = i + 1; j < bodies.size(); ++j) {
                        auto aij = gravAccel(*bodies[i], *bodies[j], G);
                        auto aji = gravAccel(*bodies[j], *bodies[i], G);
                        acc[i] += aij;
                        acc[j] += aji;
                    }
                    if(bodies[i]->name == "spaceship") {
                        acc[i] += spaceship_force / bodies[i]->mass;
                    }
                }
                for (size_t i = 0; i < bodies.size(); ++i) {
                    bodies[i]->velocity = (bodies[i]->velocity + acc[i] * step);
                    bodies[i]->position += bodies[i]->velocity * step;
                }
            }

            sampleAccum += dtFixed * timeScale;
            if(drawOrbit && sampleAccum >= sampleEvery) {
                sampleAccum = 0.f;
                for(auto& b : bodies) {
                    if(b->trail.empty() || b->trail.back() != b->position) {
                        b->trail.push_back(b->position);
                        if(b->trail.size() > b->trailMax) b->trail.pop_front();
                    }
                }
            }
        }

        Body* center = root->find(viewItems[currentView].c_str());
        if(center != nullptr) view.setCenter({static_cast<float>(center->position.x), static_cast<float>(center->position.y)});

        window.clear(sf::Color::Black);
        window.setView(view);

        if(drawOrbit) {
            for(const auto& b : bodies) {
                if (b->trail.size() < 2) continue;

                sf::VertexArray strip(sf::PrimitiveType::LineStrip, b->trail.size() + 1);
                const std::size_t n = b->trail.size();
                for (std::size_t i = 0; i < n; ++i) {
                    strip[i].position = {static_cast<float>(b->trail[i].x), static_cast<float>(b->trail[i].y)};

                    strip[i].color = sf::Color(b->color.r, b->color.g, b->color.b, 255);
                }
                strip[n].position = {static_cast<float>(b->position.x), static_cast<float>(b->position.y)};
                strip[n].color = sf::Color(b->color.r, b->color.g, b->color.b, 255);
                window.draw(strip);
            }
        }

        for (auto& b : bodies) {
            sf::CircleShape c(b->radius);
            c.setOrigin({static_cast<float>(b->radius), static_cast<float>(b->radius)});
            c.setPosition({static_cast<float>(b->position.x), static_cast<float>(b->position.y)});
            c.setFillColor(b->color);
            window.draw(c);
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}