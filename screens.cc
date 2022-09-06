#include <vector>
#include <array>

struct Vec2 {
    float x, y;
};

struct Quad{
    std::array<Vec2,4> points;
};

struct Screen {
    std::vector<Screen*> screens;
    Quad quad;
};

void draw(const Quad& q) {}



std::vector<Screen> screens;

Quad distort(Quad& view, Quad& q){

}


void render(const std::vector<Screen*>& screens, int iteration, const Quad& view) {
    if(iteration == 0){
        return;
    }

    for(auto& s : screens){
        auto newQ = distort(view, s->quad);
        draw(newQ);

        render(s->screens, iteration-1, view, newQ);
    }



}