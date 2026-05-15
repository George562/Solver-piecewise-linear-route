#include "app.h"

int main() {
    // std::pair<sf::Vector2i, sf::Vector2i> bounders = {{0, 0}, {15, 15}};
    // LineCoeffs coeff1 = {-0.51996, -1.93123, 7.87353};
    // LineCoeffs coeff2 = {-0.51996, 1.93123, -3.71385};
    // sf::Vector2i c = {4, 3};
    // sf::Vector2f m1 = {5.534, 5.546}, m2 = {5.516, 0.4643};
    // float r = 2.93781;
    // rectBy2CircleAnd2Lines(bounders, coeff1, coeff2, c, m1, m2, r, std::max, std::min);
    // std::cout << bounders.first.x << " " << bounders.first.y << " " << bounders.second.x << " " << bounders.second.y << std::endl;
    App myApp;
    myApp.init();
    myApp.generateField(myFunction);
    myApp.run();
    return 0;
}