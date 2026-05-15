#include "tools.h"

#define isInsideGridV(v) (v.x >= 0 && v.x < gridSize.x && v.y >= 0 && v.y < gridSize.y)
#define isInsideGrid(X, Y) (X >= 0 && X < gridSize.x && Y >= 0 && Y < gridSize.y)
#define isInsideField(v) (fieldSprite.getGlobalBounds().contains(v))

class App {
public:
    App(float scw = 1200.f, float sch = 890.f);
    ~App();
    void init(float angle = 60.f);
    void run();
    void draw();
    void drawField();
    void eventHandler();
    void updateField(int x, int y, float UpOrDown);
    void generateField(float (*f)(sf::Vector2f));
    void loadField(std::string);
    sf::Vector2i pos2cell(sf::Vector2f pos);
    void calculateOptimalSolution();
    void calculateOptimalSolutionWithFullFieldWalk();
    void calculateOptimalSolutionByApproxWithRect();
    void calculateOptimalSolutionByAngle();
    void calculateOptimalSolutionExperiment();
    void makeExperiment();

    sf::Image icon;
    sf::RenderWindow* window;
    float scw, sch;
    std::vector<std::vector<float>> grid;
    sf::Vector2i gridSize;
    sf::Clock* globalClock;
    sf::Time globalClockTime, TimeSinceLastFrame;
    float maxDifference;
    float cellSize = 4.f;
    float phi_0;
    sf::Vector2f fieldSize;
    sf::Vertex cell;
    sf::RenderTexture fieldTexture;
    sf::Sprite fieldSprite;
    bool acceptable;

    sf::Vector2f gradientOfMouse;
    std::vector<sf::Vector2f> gradientsOfWays, gradientOfTurns;

    sf::Text* information, cellInfo, *rangeInfo, *sliderValue, *wayInfo, *resultText, *workInfo;
    std::stringstream wayMessage;

    Slider* differenceSlider, *angleSlider;
    bool isDifferenceSliderActive = false, isAngleSliderActive = false;

    Button* generateButton, *printButton, *addTurnsButton, *removeTurnsButton, *findOptimal1, *findOptimalButton, *findOptimal0, *experimentalButton, *loadButton;
    std::vector<MyCircle*> circles;
    MyCircle* activeCircle = nullptr;

    SwitchButton* changeFieldSwitch;

    MultiSlider* multiSlider;
    float startValue, endValue, mouseValue;

    sf::VertexArray *wayLines;

    TextBox *fileNameTextBox;

    sf::Thread *threadApproxRect, *thread, *threadFullWalk, *threadExperiment;
};

App::App(float scw, float sch) {
    this->scw = scw;
    this->sch = sch;
    gridSize = sf::Vector2i(600 / cellSize, 600 / cellSize);
    fieldSize = sf::Vector2f(gridSize) * cellSize;
    grid.assign(gridSize.x, std::vector<float>(gridSize.y, 0.f));
}

App::~App() {
    delete window;
    delete globalClock;
    delete information; delete rangeInfo; delete sliderValue; delete wayInfo; delete resultText; delete workInfo;
    delete differenceSlider; delete angleSlider;
    delete generateButton; delete printButton; delete addTurnsButton, delete removeTurnsButton; delete findOptimal1; delete findOptimalButton; delete findOptimal0; delete experimentalButton; delete loadButton;
    for (int i = 0; i < circles.size(); i++) delete circles[i];
    delete changeFieldSwitch;
    delete multiSlider;
    delete wayLines;
    delete fileNameTextBox;
    thread->terminate();
    threadApproxRect->terminate();
    threadFullWalk->terminate();
    threadExperiment->terminate();
    delete threadApproxRect; delete thread; delete threadFullWalk; delete threadExperiment;
}

void App::init(float angle) {
    arialFont.loadFromFile("arial.ttf");
    consolaFont.loadFromFile("consola.ttf");

    window = new sf::RenderWindow(sf::VideoMode(scw, sch), "Solver piecewise linear route", sf::Style::Close);
    icon.loadFromFile("mai.png");
    window->setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    globalClock = new sf::Clock();
    phi_0 = angle;
    maxDifference = 0.f;

    fieldTexture.create(gridSize.x, gridSize.y);
    fieldSprite.setTexture(fieldTexture.getTexture(), true);
    fieldSprite.setScale(cellSize, cellSize);

    // shape
    circles.push_back(new MyCircle(sf::Vector2f(fieldSize.y + 15.f, fieldSize.y - 50.f), sf::Color::Red, "Start"));
    circles.push_back(new MyCircle(sf::Vector2f(fieldSize.y + 15.f, fieldSize.y - 20.f), sf::Color::Blue, "End"));

    // text
    information = new sf::Text("RMB/LMB - raise/lower the place\nMax difference = " + floatToString(maxDifference) +
                               "\n\nMax Angle = " + floatToString(phi_0, 2), arialFont, 30);
    information->setPosition(fieldSize.y + 15, 15);

    sliderValue = new sf::Text("", consolaFont, 30);
    sliderValue->setOutlineColor(sf::Color::Black);
    sliderValue->setOutlineThickness(3.f);

    cellInfo.setFont(consolaFont);
    cellInfo.setPosition(fieldSize.y + 15, 190);

    rangeInfo = new sf::Text(sf::String(L"0 ≤ x < " + std::to_wstring(gridSize.x) + L"\n0 ≤ y < " + std::to_wstring(gridSize.y)), arialFont, 30);
    rangeInfo->setPosition(fieldSize.y + 250, 190);

    wayInfo = new sf::Text("", consolaFont, 30);
    wayInfo->setPosition(fieldSize.y + 15, 360);

    resultText = new sf::Text("", consolaFont, 30);
    resultText->setPosition(fieldSize.y + 15, 320);

    workInfo = new sf::Text("Time:\nDone:", consolaFont, 30);
    workInfo->setPosition(15, fieldSize.y + 210);

    // slider
    differenceSlider = new Slider(sf::Vector2f(fieldSize.y + 20, 95), sf::Vector2f(scw - fieldSize.y - 160, 20), maxDifference, 0.f, 0.2f);
    differenceSlider->setColors(sf::Color::Red, sf::Color::White, sf::Color(100, 100, 100));
    angleSlider = new Slider(sf::Vector2f(fieldSize.y + 20, 165), sf::Vector2f(90.f * 4.f, 20), phi_0, 0.f, 90.f);
    angleSlider->setColors(sf::Color::Red, sf::Color::White, sf::Color(100, 100, 100));

    // button
    generateButton = new Button(sf::Vector2f(15, fieldSize.y + 70), "Generate grid");
    loadButton = new Button(sf::Vector2f(230, fieldSize.y + 70), "Load grid");
    addTurnsButton = new Button(sf::Vector2f(15, fieldSize.y + 15), "Add turns");
    removeTurnsButton = new Button(sf::Vector2f(178,fieldSize.y + 15), "Remove turns");
    findOptimal0 = new Button(sf::Vector2f(15, fieldSize.y + 170), "Method 0");
    findOptimal1 = new Button(sf::Vector2f(170, fieldSize.y + 170), "Method 1");
    findOptimalButton = new Button(sf::Vector2f(322, fieldSize.y + 170), "Method 2");
    experimentalButton = new Button(sf::Vector2f(474, fieldSize.y + 170), "Experiment");
    printButton = new Button(sf::Vector2f(fieldSize.y + 15, 275), "Print way");

    changeFieldSwitch = new SwitchButton(sf::Vector2f(15, fieldSize.y + 130), "Change grid");

    // multiSlider
    multiSlider = new MultiSlider(sf::Vector2f(scw - 125.f, 15.f), sf::Vector2f(20.f, sch - 30.f), 0.f, 1.f);
    startValue = endValue = mouseValue = -1.f;
    multiSlider->addValue(&mouseValue, sf::Color(0, 200, 0));
    multiSlider->addValue(&startValue, sf::Color::Red);
    multiSlider->addValue(&endValue, sf::Color::Blue);

    // lines
    wayLines = new sf::VertexArray(sf::LinesStrip, 2);
    (*wayLines)[0].color = (*wayLines)[1].color = sf::Color::Magenta;

    // TextBox
    fileNameTextBox = new TextBox(sf::Vector2f(230, fieldSize.y + 125), sf::Vector2f(200, 30));

    threadFullWalk = new sf::Thread(calculateOptimalSolutionWithFullFieldWalk, this);
    // threadApproxRect = new sf::Thread(calculateOptimalSolutionByApproxWithRect, this);
    threadApproxRect = new sf::Thread(calculateOptimalSolutionExperiment, this);
    thread = new sf::Thread(calculateOptimalSolution, this);
    threadExperiment = new sf::Thread(makeExperiment, this);

    if (cellSize <= 4.f) {
        circles[0]->setPosition(sf::Vector2f(32, gridSize.y - 1 - 22) * cellSize + sf::Vector2f(cellSize / 2.f, cellSize / 2.f));
        if (cellSize == 3.f)
            circles[1]->setPosition(sf::Vector2f(173, gridSize.y - 1 - 171) * cellSize + sf::Vector2f(cellSize / 2.f, cellSize / 2.f));
        else
            circles[1]->setPosition(sf::Vector2f(122, gridSize.y - 1 - 114) * cellSize + sf::Vector2f(cellSize / 2.f, cellSize / 2.f));
    }

    gradientsOfWays.push_back(sf::Vector2f(0.f, 0.f));
}

void App::run() {
    drawField();
    while (window->isOpen()) {
        eventHandler();
        draw();
        TimeSinceLastFrame = globalClock->getElapsedTime() - globalClockTime;
        globalClockTime = globalClock->getElapsedTime();
    }
}

void App::draw() {
    window->clear();
    window->draw(fieldSprite);
    // slider
    window->draw(*differenceSlider);
    window->draw(*angleSlider);
    // multiSlider
    window->draw(*multiSlider);
    // text
    window->draw(*information);
    window->draw(cellInfo);
    window->draw(*rangeInfo);
    window->draw(*sliderValue);
    window->draw(*wayInfo);
    window->draw(*resultText);
    window->draw(*workInfo);
    // button
    window->draw(*generateButton);
    window->draw(*loadButton);
    window->draw(*printButton);
    window->draw(*addTurnsButton);
    window->draw(*removeTurnsButton);
    window->draw(*changeFieldSwitch);
    window->draw(*findOptimal1);
    window->draw(*findOptimalButton);
    window->draw(*experimentalButton);
    window->draw(*findOptimal0);
    // TextBox
    window->draw(*fileNameTextBox);
    // line
    for (int i = 0; i < circles.size() - 1; i++) {
        if (isInsideField(circles[i]->getPosition()) && isInsideField(circles[i + 1]->getPosition())) {
            (*wayLines)[0].position = circles[i]->getPosition();
            (*wayLines)[1].position = circles[i + 1]->getPosition();
            window->draw(*wayLines);
        }
    }
    // if (gradientOfMouse != sf::Vector2f(0, 0)) {
    //     (*wayLines)[0].position = sf::Vector2f(sf::Mouse::getPosition(*window));
    //     (*wayLines)[1].position = gradientOfMouse + (*wayLines)[0].position;
    //     window->draw(*wayLines);
    // }
    // for (int i = 0; i < gradientsOfWays.size(); i++) {
    //     (*wayLines)[0].position = (circles[i]->getPosition() + circles[i + 1]->getPosition()) / 2.f;
    //     (*wayLines)[1].position = gradientsOfWays[i] + (*wayLines)[0].position;
    //     window->draw(*wayLines);
    // }
    // for (int i = 0; i < gradientOfTurns.size(); i++) {
    //     (*wayLines)[0].position = circles[i + 1]->getPosition();
    //     (*wayLines)[1].position = gradientOfTurns[i] + (*wayLines)[0].position;
    //     window->draw(*wayLines);
    // }
    // circle
    for (int i = 0; i < circles.size(); i++) window->draw(*circles[i]);
    window->display();
}

void App::drawField() {
    fieldTexture.clear();
    for (int x = 0; x < gridSize.x; x++) {
        for (int y = 0; y < gridSize.y; y++) {
            cell.position = sf::Vector2f(x, y + 1);
            cell.color = sf::Color(grid[x][y] * 255, grid[x][y] * 255, grid[x][y] * 255);
            fieldTexture.draw(&cell, 1, sf::PrimitiveType::Points);
        }
    }
    fieldTexture.display();
}

void App::eventHandler() {
    sf::Vector2f MousePos(sf::Mouse::getPosition(*window));
    sf::Vector2i CellPos(MousePos / cellSize);

    gradientOfMouse = sf::Vector2f(0, 0);
    if (isInsideField(MousePos)) {
        gradientOfMouse = gradient(grid, CellPos);
        if (gradientOfMouse.x != 0 && gradientOfMouse.y != 0) gradientOfMouse *= 4000.f;
    }
    for (int i = 0; i < gradientsOfWays.size(); i++) {
        if (isInsideField(circles[i]->getPosition()) && isInsideField(circles[i + 1]->getPosition())) {
            gradientsOfWays[i] = sf::Vector2f(0, 0);
            sf::Vector2f from(circles[i]->getPosition() / cellSize), to(circles[i + 1]->getPosition() / cellSize);
            if (to.x == from.x) {
                for (; from.y != to.y; from.y += to.y > from.y ? 1 : -1) gradientsOfWays[i] += gradient(grid, from);
                gradientsOfWays[i] = (gradientsOfWays[i] + gradient(grid, to)) * 200.f;
                continue;
            }
            if (to.y == from.y) {
                for (; from.x != to.x; from.x += to.x > from.x ? 1 : -1) gradientsOfWays[i] += gradient(grid, from);
                gradientsOfWays[i] = (gradientsOfWays[i] + gradient(grid, to)) * 200.f;
                continue;
            }
            if (from.x > to.x) std::swap(from, to);
            sf::Vector2f d(to - from), cur(from);
            float k = d.y / d.x;
            int dy = k > 0 ? 1 : -1, nextY = (cur.x - from.x + 1) * k + from.y;
            while (cur.x <= to.x && cur.y * dy <= to.y * dy) {
                gradientsOfWays[i] += gradient(grid, cur);
                if (nextY == cur.y || cur.y == to.y) {
                    cur.x++;
                    nextY = (cur.x - from.x + 1) * k + from.y;
                } else {
                    cur.y += dy;
                }
            }
            gradientsOfWays[i] *= 200.f;
        }
    }
    for (int i = 0; i < gradientOfTurns.size(); i++) {
        gradientOfTurns[i] = (gradientsOfWays[i] + gradientsOfWays[i + 1]) / 2.f;
    }

    // set info text
    std::string message = "x: " + std::to_string(CellPos.x) + "\ny: " + std::to_string((int)gridSize.y - 1 - CellPos.y);
    cellInfo.setString(!isInsideField(MousePos) ? "x:\ny:" : message);
    mouseValue = isInsideField(MousePos) ? grid[CellPos.x][CellPos.y] : -1.f;
    wayMessage.str("");
    wayMessage << "start: ";
    if (isInsideField(circles.front()->getPosition()) ) {
        wayMessage << "(" << std::setw(3) << pos2cell(circles.front()->getPosition()).x << ", " << std::setw(3) << pos2cell(circles.front()->getPosition()).y << ")";
    }
    for (int i = 1; i < circles.size() - 1; i++) {
        wayMessage << "\nturn" + std::to_string(i) + ": ";
        if (isInsideField(circles[i]->getPosition())) {
            sf::Vector2f p(circles[i]->getPosition() - circles[i - 1]->getPosition()), q(circles[i + 1]->getPosition() - circles[i]->getPosition());
            float angle = diffAngle(p, q);
            wayMessage << "(" << std::setw(3) << pos2cell(circles[i]->getPosition()).x << ", " << std::setw(3) << pos2cell(circles[i]->getPosition()).y << ") " << std::setw(5) << floatToString(angle, 2);
        }
    }
    wayMessage << "\nend:   ";
    if (isInsideField(circles.back()->getPosition()) ) {
        wayMessage << "(" << std::setw(3) << pos2cell(circles.back()->getPosition()).x << ", " << std::setw(3) << pos2cell(circles.back()->getPosition()).y << ")";
    }
    wayInfo->setString(" name    point    angle\n" + wayMessage.str());

    // set start and end points
    startValue = isInsideField(circles.front()->getPosition()) ? grid[circles.front()->getPosition().x / cellSize][circles.front()->getPosition().y / cellSize] : -1.f;
    endValue = isInsideField(circles.back()->getPosition()) ? grid[circles.back()->getPosition().x / cellSize][circles.back()->getPosition().y / cellSize] : -1.f;
    if (activeCircle) {
        activeCircle->setPosition((sf::Vector2f)CellPos * cellSize + sf::Vector2f(cellSize / 2.f, cellSize / 2.f));
    }

    acceptable = true;
    for (int i = 0; i < circles.size(); i++) {
        acceptable &= isInsideField(circles[i]->getPosition());
    }

    // set slider value
    if (isDifferenceSliderActive) {
        maxDifference = differenceSlider->upper * (MousePos.x - differenceSlider->front.getPosition().x) / differenceSlider->front.getSize().x;
        maxDifference = std::clamp(maxDifference, differenceSlider->lower, differenceSlider->upper);
        differenceSlider->setValue(maxDifference);
    }
    if (isAngleSliderActive) {
        phi_0 = angleSlider->upper * (MousePos.x - angleSlider->front.getPosition().x) / angleSlider->front.getSize().x;
        phi_0 = std::clamp(phi_0, angleSlider->lower, angleSlider->upper);
        angleSlider->setValue(phi_0);
    }
    information->setString("RMB/LMB - raise/lower the place\nMax difference = " + floatToString(maxDifference) + "\n\nMax Angle = " + floatToString(phi_0, 2));

    // set slider value text
    sliderValue->setPosition(scw, sch);
    if (differenceSlider->front.getLocalBounds().contains(sf::Vector2f(MousePos) - differenceSlider->front.getPosition())) {
        sliderValue->setString(floatToString(differenceSlider->upper * (MousePos.x - differenceSlider->front.getPosition().x) / differenceSlider->front.getSize().x));
        sliderValue->setPosition(MousePos.x - sliderValue->getLocalBounds().width / 2.f, MousePos.y - sliderValue->getLocalBounds().height - 10.f);
    }
    if (angleSlider->front.getLocalBounds().contains(sf::Vector2f(MousePos) - angleSlider->front.getPosition())) {
        sliderValue->setString(floatToString(angleSlider->upper * (MousePos.x - angleSlider->front.getPosition().x) / angleSlider->front.getSize().x, 2));
        sliderValue->setPosition(MousePos.x - sliderValue->getLocalBounds().width / 2.f, MousePos.y - sliderValue->getLocalBounds().height - 10.f);
    }

    if (acceptable && !changeFieldSwitch->isOn) {
        float res = 0.f;
        for (int i = 0; i < circles.size() - 1; i++) {
            sf::Vector2i startPos(circles[i]->getPosition() / cellSize), endPos(circles[i + 1]->getPosition() / cellSize);
            res += discreteIntegral(grid, startPos, endPos);
        }
        resultText->setString("path difficulty: " + floatToString(res));
    } else {
        resultText->setString("path difficulty: ");
    }

    //handle events
    sf::Event event;
    while (window->pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window->close();
        } else if (generateButton->isPressed(event)) {
            generateField(myFunction);
        } else if (loadButton->isPressed(event)) {
            loadField(fileNameTextBox->string);
        } else if (printButton->isPressed(event)) {
            if (acceptable && !changeFieldSwitch->isOn) {
                float res = 0.f;
                for (int i = 0; i < circles.size() - 1; i++) {
                    sf::Vector2i startPos(circles[i]->getPosition() / cellSize), endPos(circles[i + 1]->getPosition() / cellSize);
                    res += discreteIntegral(grid, startPos, endPos);
                }
                std::cout << wayMessage.str() << "\npath difficulty: " << res << '\n';
            }
        } else if (changeFieldSwitch->isPressed(event)) {
            changeFieldSwitch->Switch();
        } else if (addTurnsButton->isPressed(event)) {
            circles.insert(circles.end() - 1, new MyCircle(sf::Vector2f(gridSize / 2) * cellSize, sf::Color(0, 160, 0), "Turn" + std::to_string(circles.size() - 1)));
            gradientsOfWays.push_back(sf::Vector2f{0.f, 0.f});
            gradientOfTurns.push_back(sf::Vector2f{0.f, 0.f});
        } else if (removeTurnsButton->isPressed(event) && circles.size() > 2) {
            circles.erase(circles.end() - 2);
            gradientsOfWays.pop_back();
            gradientOfTurns.pop_back();
        } else if (acceptable && findOptimal0->isPressed(event)) {
            threadFullWalk->launch();
        } else if (acceptable && findOptimal1->isPressed(event)) {
            threadApproxRect->launch();
        } else if (acceptable && findOptimalButton->isPressed(event)) {
            thread->launch();
        } else if (acceptable && experimentalButton->isPressed(event)) {
            threadExperiment->launch();
        } else {
            fileNameTextBox->InputText(event);
            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    if (activeCircle) {
                        activeCircle->setPosition(sf::Vector2f(CellPos) * cellSize + sf::Vector2f(cellSize / 2.f, cellSize / 2.f));
                        if (!isInsideField(MousePos) && activeCircle->name.getString()[0] == 'T') {
                            circles.erase(std::find(circles.begin(), circles.end(), activeCircle));
                            for (int i = 1; i < circles.size() - 1; i++) {
                                circles[i]->name.setString("Turn" + std::to_string(i));
                            }
                        }
                        activeCircle = nullptr;
                    } else {
                        for (int i = 0; i < circles.size() && !activeCircle; i++) {
                            if (distanse(circles[i]->getPosition(), MousePos) <= MyCircle::radius) {
                                changeFieldSwitch->set(false);
                                activeCircle = circles[i];
                            }
                        }
                    }
                    isDifferenceSliderActive = differenceSlider->background.getGlobalBounds().contains(MousePos);
                    isAngleSliderActive = angleSlider->background.getGlobalBounds().contains(MousePos);
                }
            } else if (event.type == sf::Event::MouseButtonReleased) {
                isDifferenceSliderActive = isAngleSliderActive = false;
            }
        }
    }

    //update grid
    if (changeFieldSwitch->isOn &&sf::Mouse::isButtonPressed(sf::Mouse::Left) || sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
        updateField(CellPos.x, CellPos.y, sf::Mouse::isButtonPressed(sf::Mouse::Left) ? 1.f : -1.f);
    }
}

void App::updateField(int x, int y, float UpOrDown) {
    if (!isInsideGrid(x, y)) return;
    grid[x][y] = std::clamp(grid[x][y] + TimeSinceLastFrame.asSeconds() * UpOrDown, 0.f, 1.f);

    cell.position = sf::Vector2f(x, y);
    cell.color = sf::Color(grid[x][y] * 255, grid[x][y] * 255, grid[x][y] * 255);
    fieldTexture.draw(&cell, 1, sf::PrimitiveType::Points);

    std::queue<sf::Vector2i> cells; cells.push(sf::Vector2i(x, y));
    std::vector<std::vector<bool>> used(gridSize.x, std::vector<bool>(gridSize.y, false));
    while (!cells.empty()) {
        sf::Vector2i pos = cells.front(); cells.pop();
        float curValue = grid[pos.x][pos.y];
        if (used[pos.x][pos.y]) continue;
        used[pos.x][pos.y] = true;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                int nx = pos.x + dx, ny = pos.y + dy;
                if ((dx == 0 && dy == 0) || !isInsideGrid(nx, ny) || used[nx][ny]) continue;
                float& nextValue = grid[nx][ny];
                if (abs(curValue - nextValue) > maxDifference) {
                    if (std::abs(nextValue - (curValue - maxDifference)) < std::abs(nextValue - (curValue + maxDifference))) {
                        nextValue = std::clamp(curValue - maxDifference, 0.f, 1.f);
                    } else {
                        nextValue = std::clamp(curValue + maxDifference, 0.f, 1.f);
                    }
                    cell.position = sf::Vector2f(nx, ny);
                    cell.color = sf::Color(nextValue * 255, nextValue * 255, nextValue * 255);
                    fieldTexture.draw(&cell, 1, sf::PrimitiveType::Points);

                    cells.push(sf::Vector2i(nx, ny));
                }
            }
        }
    }
    fieldTexture.display();
}

void App::generateField(float (*f)(sf::Vector2f)) {
    maxDifference = 0.f;
    for (int x = 0; x < gridSize.x; x++) {
        for (int y = 0; y < gridSize.y; y++) {
            grid[x][y] = std::pow(std::clamp((*f)(sf::Vector2f(x, (int)gridSize.y - 1 - y)), 0.f, 1.f), 1.f);
            if (x > 0 && y > 0) {
                maxDifference = std::max(maxDifference, std::abs(grid[x][y] - grid[x - 1][y - 1]));
                maxDifference = std::max(maxDifference, std::abs(grid[x][y] - grid[x    ][y - 1]));
                maxDifference = std::max(maxDifference, std::abs(grid[x][y] - grid[x - 1][y    ]));
            }
        }
    }
    differenceSlider->setValue(maxDifference);
    drawField();

    XcumsumGrid.resize(gridSize.x, std::vector<float>(gridSize.y));
    YcumsumGrid.resize(gridSize.x, std::vector<float>(gridSize.y));
    for (int x = 0; x < gridSize.x; x++) {
        for (int y = 0; y < gridSize.y; y++) {
            XcumsumGrid[x][y] = grid[x][y] + ((x == 0) ? 0 : XcumsumGrid[x - 1][y]);
            YcumsumGrid[x][y] = grid[x][y] + ((y == 0) ? 0 : YcumsumGrid[x][y - 1]);
        }
    }
}

inline void App::loadField(std::string filename) {
    sf::Image img;
    std::ifstream file(filename);
    if (!file.is_open()) return;
    img.loadFromFile(filename);
    gridSize = sf::Vector2i(img.getSize());
    cellSize = std::min((fieldSize.x / gridSize .x), (fieldSize.y / gridSize.y));
    std::cout << cellSize << std::endl;
    fieldTexture.create(gridSize.x, gridSize.y);
    fieldSprite.setTexture(fieldTexture.getTexture(), true);
    fieldSprite.setScale(cellSize, cellSize);
    grid.assign(gridSize.x, std::vector<float>(gridSize.y, 0.f));
    for (int i = 0; i < gridSize.y; i++) {
        for (int j = 0; j < gridSize.x; j++) {
            grid[j][i] = (img.getPixel(j, i).r + img.getPixel(j, i).g + img.getPixel(j, i).b) / (256.f * 3.f);
        }
    }
    rangeInfo->setString(L"0 ≤ x < " + std::to_wstring(gridSize.x) + L"\n0 ≤ y < " + std::to_wstring(gridSize.y));
    drawField();
    
    XcumsumGrid.resize(gridSize.x, std::vector<float>(gridSize.y));
    YcumsumGrid.resize(gridSize.x, std::vector<float>(gridSize.y));
    for (int x = 0; x < gridSize.x; x++) {
        for (int y = 0; y < gridSize.y; y++) {
            XcumsumGrid[x][y] = grid[x][y] + ((x == 0) ? 0 : XcumsumGrid[x - 1][y]);
            YcumsumGrid[x][y] = grid[x][y] + ((y == 0) ? 0 : YcumsumGrid[x][y - 1]);
        }
    }
    while (circles.size() > 2) {
        circles.erase(circles.end() - 2);
        gradientsOfWays.pop_back();
        gradientOfTurns.pop_back();
    }
    circles[0]->setPosition(fieldSize);
    circles[1]->setPosition(fieldSize + sf::Vector2f(0.f, 30.f));
    std::cout << "bruh\n";
}

sf::Vector2i App::pos2cell(sf::Vector2f pos) {
    return sf::Vector2i(pos.x / cellSize, gridSize.x - pos.y / cellSize);
}

void App::calculateOptimalSolutionWithFullFieldWalk() { // добавить очередь с приоритетом для первой вершины
    if (circles.size() < 3) return;
    findOptimal0->text.setFillColor(sf::Color::Red);
    const auto start = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> diff;

    sf::Vector2i startPoint(circles.front()->getPosition() / cellSize), endPoint(circles.back()->getPosition() / cellSize), corner(0, 0), leftSize(0, 0);

    std::vector<std::vector<float>> precalcIntegralsToEnd(gridSize.x, std::vector<float>(gridSize.y, -1.f));
    for (sf::Vector2i v = sf::Vector2i(0, 0); v.x < gridSize.x; v.x++)
        for (v.y = 0; v.y < gridSize.y; v.y++)
            precalcIntegralsToEnd[v.x][v.y] = discreteIntegral(grid, v, endPoint);

    int PATHSIZE = circles.size() - 1;
    std::vector<sf::Vector2i> path(PATHSIZE); path[0] = startPoint; path[1] = corner;
    int pathSize = 2;
    std::vector<float> precalcLength(PATHSIZE, 0.f);
    float res = discreteIntegral(grid, startPoint, endPoint), curRes = 0.f;
    std::vector<float> cosMaxAngle(PATHSIZE + 1); int cosActuality;
    for (cosActuality = PATHSIZE; cosActuality > 0 && (PATHSIZE - cosActuality + 1) * phi_0 < 180.f; cosActuality--) {
        cosMaxAngle[cosActuality] = std::cos((PATHSIZE - cosActuality + 1) * M_PI * phi_0 / 180.f);
    }
    /* best score for grid 150x150, angle 60 degree
    Optimal solution found in 3.20831 seconds.
    start: ( 32,  22)
    turn1: ( 96,  20) 46.78
    turn2: (125,  49) 47.64
    end:   (122, 114)
    path difficulty: 93.635
    */
    sf::Thread thr([&]{
        while (true) {
            diff = std::chrono::high_resolution_clock::now() - start;
            workInfo->setString("Time: " + floatToString(diff.count(), 2) + " sec\nDone: " + floatToString(100 * float(path[1].x + path[1].y * gridSize.y) / gridSize.x / gridSize.y, 2) + " %");
        }
    });
    thr.launch();
    while (pathSize > 1) {
        if ((pathSize > 2 && normDot(path[pathSize - 1] - path[pathSize - 2], path[pathSize - 2] - path[pathSize - 3]) < cosMaxAngle[PATHSIZE]) ||
            (pathSize > cosActuality && normDot(endPoint - path[pathSize - 1], path[pathSize - 1] - path[pathSize - 2]) < cosMaxAngle[pathSize]) ||
            path[pathSize - 1] == path[pathSize - 2] || path[pathSize - 1] == endPoint) {
            goto doStep;
        }
        precalcLength[pathSize - 1] = precalcLength[pathSize - 2] + discreteIntegral(grid, path[pathSize - 2], path[pathSize - 1]);
        if (precalcLength[pathSize - 1] < res) {
            if (pathSize < PATHSIZE) {
                pathSize++;
                path[pathSize - 1] = corner;
                continue;
            }
            curRes = precalcLength[pathSize - 1] + precalcIntegralsToEnd[path[pathSize - 1].x][path[pathSize - 1].y];
            if (curRes < res) {
                res = curRes;
                for (int i = 1; i < pathSize; i++) {
                    circles[i]->setPosition((sf::Vector2f)path[i] * cellSize + sf::Vector2f(cellSize / 2.f, cellSize / 2.f));
                }
            }
        }
        doStep:
        if (path[pathSize - 1].x < gridSize.x - 1) {
            path[pathSize - 1].x++;
        } else {
            path[pathSize - 1].y++;
            path[pathSize - 1].x = 0;
        }
        if (path[pathSize - 1].y == gridSize.y) {
            pathSize--;
            goto doStep;
        }
    }

    diff = std::chrono::high_resolution_clock::now() - start;
    std::cout << "Optimal solution found by Method 0 in " << diff.count() << " seconds.\n";
    findOptimal0->text.setFillColor(sf::Color::Black);
    thr.terminate();
}

void App::calculateOptimalSolutionByApproxWithRect() { // добавить очередь с приоритетом для первой вершины
    if (circles.size() < 3) return;
    findOptimal1->text.setFillColor(sf::Color::Red);
    const auto start = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> diff;

    sf::Vector2i startPoint(circles.front()->getPosition() / cellSize), endPoint(circles.back()->getPosition() / cellSize), corner(0, 0), leftSize(0, 0);

    std::vector<std::vector<float>> precalcIntegralsToEnd(gridSize.x, std::vector<float>(gridSize.y, -1.f)), distanseToEnd(gridSize.x, std::vector<float>(gridSize.y, -1.f));
    for (sf::Vector2i v = sf::Vector2i(0, 0); v.x < gridSize.x; v.x++)
        for (v.y = 0; v.y < gridSize.y; v.y++) {
            precalcIntegralsToEnd[v.x][v.y] = discreteIntegral(grid, v, endPoint);
            distanseToEnd[v.x][v.y] = distanse(v, endPoint);
        }

    float maxAngle = phi_0 * M_PI / 180, cosOfMaxAngle = std::cos(maxAngle), sinOfMaxAngle = std::sin(maxAngle);
    int PATHSIZE = circles.size() - 1;
    std::vector<sf::Vector2i> path(PATHSIZE + 1); path[0] = startPoint;  path[PATHSIZE] = endPoint;
    std::vector<std::pair<sf::Vector2i, sf::Vector2i>> bounders(PATHSIZE);
    sf::Vector2f m1, m2, perp;
    float radius;
    std::vector<float> tans(PATHSIZE), sins(PATHSIZE);
    for (int i = 1; i < PATHSIZE; i++) {
        tans[i] = 2.f * std::tan((PATHSIZE - i) * maxAngle);
        sins[i] = 2.f * std::sin((PATHSIZE - i) * maxAngle);
    }

    if ((PATHSIZE - 1) * phi_0 >= 180) {
        bounders[1].first = sf::Vector2i(0, 0);
        bounders[1].second = gridSize;
    } else {
        m2 = (sf::Vector2f)(path[0] + endPoint) / 2.f;
        perp = (sf::Vector2f)perpendicular(endPoint - path[0]) / tans[1];
        m1 = m2 + perp;
        m2 = m2 - perp;
        radius = distanseToEnd[path[0].x][path[0].y] / sins[1];
        if ((PATHSIZE - 1) * phi_0 >= 90) {
            bounders[1].first.x = std::clamp(std::min(m1.x, m2.x) - radius, 0.f, (float)gridSize.x);
            bounders[1].first.y = std::clamp(std::min(m1.y, m2.y) - radius, 0.f, (float)gridSize.y);
            bounders[1].second.x = std::clamp(std::max(m1.x, m2.x) + radius, 0.f, (float)gridSize.x);
            bounders[1].second.y = std::clamp(std::max(m1.y, m2.y) + radius, 0.f, (float)gridSize.y);
        } else {
            if (std::max(m1.y, m2.y) > std::max(path[0].y, endPoint.y)) {
                bounders[1].first.x = std::clamp(std::max(std::max(m1.x, m2.x) - radius, (float)std::min(path[0].x, endPoint.x)), 0.f, (float)gridSize.x);
                bounders[1].second.x = std::clamp(std::min(std::min(m1.x, m2.x) + radius, (float)std::max(path[0].x, endPoint.x)), 0.f, (float)gridSize.x);
            } else {
                bounders[1].first.x = std::clamp(std::max(m1.x, m2.x) - radius, 0.f, (float)gridSize.x);
                bounders[1].second.x = std::clamp(std::min(m1.x, m2.x) + radius, 0.f, (float)gridSize.x);
            }
            if (std::max(m1.x, m2.x) > std::max(path[0].x, endPoint.x)) {
                bounders[1].first.y = std::clamp(std::max(std::max(m1.y, m2.y) - radius, (float)std::min(path[0].y, endPoint.y)), 0.f, (float)gridSize.y);
                bounders[1].second.y = std::clamp(std::min(std::min(m1.y, m2.y) + radius, (float)std::max(path[0].y, endPoint.y)), 0.f, (float)gridSize.y);
            } else {
                bounders[1].first.y = std::clamp(std::max(m1.y, m2.y) - radius, 0.f, (float)gridSize.y);
                bounders[1].second.y = std::clamp(std::min(m1.y, m2.y) + radius, 0.f, (float)gridSize.y);
            }
        }
    }
    path[1] = bounders[1].first;
    int pathSize = 2;
    std::vector<float> precalcLength(PATHSIZE, 0.f);
    float res = discreteIntegral(grid, startPoint, endPoint), curRes = 0.f;
    std::vector<float> coss(PATHSIZE + 1); int cosActuality;
    for (cosActuality = PATHSIZE; cosActuality > 0 && (PATHSIZE - cosActuality + 1) * phi_0 < 180.f; cosActuality--) {
        coss[cosActuality] = std::cos((PATHSIZE - cosActuality + 1) * maxAngle);
    }
    std::vector<std::pair<LineCoeffs, LineCoeffs>> coeffs(PATHSIZE); // precalcCoeffsOfLines
    /* best score for grid 150x150, angle 60 degree
    Optimal solution found in 1.30134 seconds.
    start: ( 32,  22)
    turn1: ( 96,  20) 46.78
    turn2: (125,  49) 47.64
    end:   (122, 114)
    path difficulty: 93.635

    best score for grid 200x200, angle 60 degree
    Optimal solution found by Method 2.2 in 12.9422 seconds.
    start: ( 32,  22)
    turn1: (112,  17) 31.75
    turn2: (168,  47) 59.51
    end:   (173, 171)
    path difficulty: 115.086
    */
    sf::Thread thr([&]{
        while (true) {
            diff = std::chrono::high_resolution_clock::now() - start;
            workInfo->setString("Time: " + floatToString(diff.count(), 2) + " sec\nDone: " + floatToString(100 * float(path[1].x - bounders[1].first.x + (path[1].y - bounders[1].first.y) * (bounders[1].second.x - bounders[1].first.x)) / (bounders[1].second.x - bounders[1].first.x) / (bounders[1].second.y - bounders[1].first.y), 2) + " %");
        }
    });
    thr.launch();
    // long long counter = 0;
    while (true) {
        // counter++;
        if ((pathSize > 2 && normDot(path[pathSize - 1] - path[pathSize - 2], path[pathSize - 2] - path[pathSize - 3]) < coss[PATHSIZE]) ||
            (pathSize > cosActuality && normDot(endPoint - path[pathSize - 1], path[pathSize - 1] - path[pathSize - 2]) < coss[pathSize]) ||
            path[pathSize - 1] == path[pathSize - 2] || path[pathSize - 1] == endPoint) {
            goto doStep;
        }
        precalcLength[pathSize - 1] = precalcLength[pathSize - 2] + discreteIntegral(grid, path[pathSize - 2], path[pathSize - 1]);
        if (precalcLength[pathSize - 1] < res) {
            if (pathSize < PATHSIZE) {
                if (pathSize <= cosActuality) {
                    bounders[pathSize].first = sf::Vector2i(0, 0);
                    bounders[pathSize].second = gridSize;
                } else {
                    m2 = (sf::Vector2f)(path[pathSize - 1] + endPoint) / 2.f;
                    perp = (sf::Vector2f)perpendicular(endPoint - path[pathSize - 1]) / tans[pathSize];
                    m1 = m2 + perp;
                    m2 = m2 - perp;
                    radius = distanseToEnd[path[pathSize - 1].x][path[pathSize - 1].y] / sins[pathSize];
                    if ((PATHSIZE - pathSize) * phi_0 >= 90) {
                        bounders[pathSize].first.x = std::min(m1.x, m2.x) - radius;
                        bounders[pathSize].first.y = std::min(m1.y, m2.y) - radius;
                        bounders[pathSize].second.x = std::max(m1.x, m2.x) + radius;
                        bounders[pathSize].second.y = std::max(m1.y, m2.y) + radius;
                    } else {
                        if (std::max(m1.y, m2.y) > std::max(path[pathSize - 1].y, endPoint.y)) {
                            bounders[pathSize].first.x = std::max(std::max(m1.x, m2.x) - radius, (float)std::min(path[pathSize - 1].x, endPoint.x));
                            bounders[pathSize].second.x = std::min(std::min(m1.x, m2.x) + radius, (float)std::max(path[pathSize - 1].x, endPoint.x));
                        } else {
                            bounders[pathSize].first.x = std::max(m1.x, m2.x) - radius;
                            bounders[pathSize].second.x = std::min(m1.x, m2.x) + radius;
                        }
                        if (std::max(m1.x, m2.x) > std::max(path[pathSize - 1].x, endPoint.x)) {
                            bounders[pathSize].first.y = std::max(std::max(m1.y, m2.y) - radius, (float)std::min(path[pathSize - 1].y, endPoint.y));
                            bounders[pathSize].second.y = std::min(std::min(m1.y, m2.y) + radius, (float)std::max(path[pathSize - 1].y, endPoint.y));
                        } else {
                            bounders[pathSize].first.y = std::max(m1.y, m2.y) - radius;
                            bounders[pathSize].second.y = std::min(m1.y, m2.y) + radius;
                        }
                    }
                }
                linesByPoints(coeffs[pathSize], path[pathSize - 2], path[pathSize - 1], sinOfMaxAngle, cosOfMaxAngle);
                subrectByLine(bounders[pathSize], coeffs[pathSize].first);
                subrectByLine(bounders[pathSize], coeffs[pathSize].second);
                bounders[pathSize].first.x = std::clamp(bounders[pathSize].first.x, 0, gridSize.x);
                bounders[pathSize].first.y = std::clamp(bounders[pathSize].first.y, 0, gridSize.y);
                bounders[pathSize].second.x = std::clamp(bounders[pathSize].second.x, 0, gridSize.x);
                bounders[pathSize].second.y = std::clamp(bounders[pathSize].second.y, 0, gridSize.y);
                pathSize++;
                path[pathSize - 1] = bounders[pathSize - 1].first;
                continue;
            }
            curRes = precalcLength[pathSize - 1] + precalcIntegralsToEnd[path[pathSize - 1].x][path[pathSize - 1].y];
            if (curRes < res) {
                res = curRes;
                for (int i = 1; i < pathSize; i++) {
                    circles[i]->setPosition((sf::Vector2f)path[i] * cellSize + sf::Vector2f(cellSize / 2.f, cellSize / 2.f));
                }
            }
        }
        doStep:
        if (path[pathSize - 1].x < bounders[pathSize - 1].second.x - 1) {
            path[pathSize - 1].x++;
        } else {
            path[pathSize - 1].y++;
            path[pathSize - 1].x = bounders[pathSize - 1].first.x;
        }
        if (path[pathSize - 1].y >= bounders[pathSize - 1].second.y) {
            pathSize--;
            if (pathSize > 1) goto doStep;
            else break;
        }
    }
    // std::cout << "counter = " << counter << std::endl;

    diff = std::chrono::high_resolution_clock::now() - start;
    std::cout << "Optimal solution found by Method 1 in " << diff.count() << " seconds.\n";
    findOptimal1->text.setFillColor(sf::Color::Black);
    thr.terminate();
}

void App::calculateOptimalSolutionExperiment() {
    if (circles.size() < 3) return;
    findOptimal1->text.setFillColor(sf::Color::Red);
    const auto start = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> diff;

    sf::Vector2i startPoint(circles.front()->getPosition() / cellSize), endPoint(circles.back()->getPosition() / cellSize), corner(0, 0), leftSize(0, 0);

    std::vector<std::vector<float>> precalcIntegralsToEnd(gridSize.x, std::vector<float>(gridSize.y, -1.f)), distanseToEnd(gridSize.x, std::vector<float>(gridSize.y, -1.f));
    for (sf::Vector2i v = sf::Vector2i(0, 0); v.x < gridSize.x; v.x++)
        for (v.y = 0; v.y < gridSize.y; v.y++) {
            precalcIntegralsToEnd[v.x][v.y] = discreteIntegral(grid, v, endPoint);
            distanseToEnd[v.x][v.y] = distanse(v, endPoint);
        }

    float maxAngle = phi_0 * M_PI / 180, cosOfMaxAngle = std::cos(maxAngle), sinOfMaxAngle = std::sin(maxAngle);
    int PATHSIZE = circles.size() - 1;
    std::vector<sf::Vector2i> path(PATHSIZE + 1); path[0] = startPoint;  path[PATHSIZE] = endPoint;
    std::vector<std::pair<sf::Vector2i, sf::Vector2i>> bounders(PATHSIZE);
    sf::Vector2f m1, m2, perp;
    float radius;
    std::vector<float> tans(PATHSIZE), sins(PATHSIZE);
    for (int i = 1; i < PATHSIZE; i++) {
        tans[i] = 2.f * std::tan((PATHSIZE - i) * maxAngle);
        sins[i] = 2.f * std::sin((PATHSIZE - i) * maxAngle);
    }

    if ((PATHSIZE - 1) * phi_0 >= 180) {
        bounders[1].first = sf::Vector2i(0, 0);
        bounders[1].second = gridSize;
    } else {
        m2 = (sf::Vector2f)(path[0] + endPoint) / 2.f;
        perp = (sf::Vector2f)perpendicular(endPoint - path[0]) / tans[1];
        m1 = m2 + perp;
        m2 = m2 - perp;
        radius = distanseToEnd[path[0].x][path[0].y] / sins[1];
        if ((PATHSIZE - 1) * phi_0 >= 90) {
            bounders[1].first.x = std::clamp(std::min(m1.x, m2.x) - radius, 0.f, (float)gridSize.x);
            bounders[1].first.y = std::clamp(std::min(m1.y, m2.y) - radius, 0.f, (float)gridSize.y);
            bounders[1].second.x = std::clamp(std::max(m1.x, m2.x) + radius, 0.f, (float)gridSize.x);
            bounders[1].second.y = std::clamp(std::max(m1.y, m2.y) + radius, 0.f, (float)gridSize.y);
        } else {
            if (std::max(m1.y, m2.y) > std::max(path[0].y, endPoint.y)) {
                bounders[1].first.x = std::clamp(std::max(std::max(m1.x, m2.x) - radius, (float)std::min(path[0].x, endPoint.x)), 0.f, (float)gridSize.x);
                bounders[1].second.x = std::clamp(std::min(std::min(m1.x, m2.x) + radius, (float)std::max(path[0].x, endPoint.x)), 0.f, (float)gridSize.x);
            } else {
                bounders[1].first.x = std::clamp(std::max(m1.x, m2.x) - radius, 0.f, (float)gridSize.x);
                bounders[1].second.x = std::clamp(std::min(m1.x, m2.x) + radius, 0.f, (float)gridSize.x);
            }
            if (std::max(m1.x, m2.x) > std::max(path[0].x, endPoint.x)) {
                bounders[1].first.y = std::clamp(std::max(std::max(m1.y, m2.y) - radius, (float)std::min(path[0].y, endPoint.y)), 0.f, (float)gridSize.y);
                bounders[1].second.y = std::clamp(std::min(std::min(m1.y, m2.y) + radius, (float)std::max(path[0].y, endPoint.y)), 0.f, (float)gridSize.y);
            } else {
                bounders[1].first.y = std::clamp(std::max(m1.y, m2.y) - radius, 0.f, (float)gridSize.y);
                bounders[1].second.y = std::clamp(std::min(m1.y, m2.y) + radius, 0.f, (float)gridSize.y);
            }
        }
    }
    path[1] = bounders[1].first;
    int pathSize = 2;
    std::vector<float> precalcLength(PATHSIZE, 0.f);
    float res = discreteIntegral(grid, startPoint, endPoint), curRes = 0.f;
    std::vector<float> coss(PATHSIZE + 1); int cosActuality;
    for (cosActuality = PATHSIZE; cosActuality > 0 && (PATHSIZE - cosActuality + 1) * phi_0 < 180.f; cosActuality--) {
        coss[cosActuality] = std::cos((PATHSIZE - cosActuality + 1) * maxAngle);
    }
    // std::cout << "cosActuality: " << cosActuality << std::endl;
    std::pair<LineCoeffs, LineCoeffs> coeffs; // precalcCoeffsOfLines
    std::vector<float (*)(float&, float&, float&)> Lcomp(PATHSIZE), Rcomp(PATHSIZE);
    for (int i = 2; (PATHSIZE - i) * phi_0 > 90.f; i++) {
        Lcomp[i] = [](float& x, float& y, float& z) { return std::min(x, y); };
        Rcomp[i] = [](float& x, float& y, float& z) { return std::max(x, y); };
    }
    for (int i = PATHSIZE - 1; i > 1 && (PATHSIZE - i) * phi_0 <= 90.f; i--) {
        Lcomp[i] = [](float& x, float& y, float& z) { return std::max(std::max(x, y), z); };
        Rcomp[i] = [](float& x, float& y, float& z) { return std::min(std::min(x, y), z); };
    }
    /* best score for grid 150x150, angle 60 degree
    Optimal solution found in 1.30417 seconds.
    start: ( 32,  22)
    turn1: ( 96,  20) 46.78
    turn2: (125,  49) 47.64
    end:   (122, 114)
    path difficulty: 93.635

    best score for grid 200x200, angle 60 degree
    Optimal solution found by Method 2.2 in 11.7923 seconds.
    start: ( 32,  22)
    turn1: (112,  17) 31.75
    turn2: (168,  47) 59.51
    end:   (173, 171)
    path difficulty: 115.086
    */
    sf::Thread thr([&]{
        while (true) {
            diff = std::chrono::high_resolution_clock::now() - start;
            workInfo->setString("Time: " + floatToString(diff.count(), 2) + " sec\nDone: " + floatToString(100 * float(path[1].x - bounders[1].first.x + (path[1].y - bounders[1].first.y) * (bounders[1].second.x - bounders[1].first.x)) / (bounders[1].second.x - bounders[1].first.x) / (bounders[1].second.y - bounders[1].first.y), 2) + " %");
        }
    });
    thr.launch();
    // long long counter = 0;
    while (true) {
        // counter++;
        if ((pathSize > 2 && normDot(path[pathSize - 1] - path[pathSize - 2], path[pathSize - 2] - path[pathSize - 3]) < coss[PATHSIZE]) ||
            (pathSize > cosActuality && normDot(endPoint - path[pathSize - 1], path[pathSize - 1] - path[pathSize - 2]) < coss[pathSize]) ||
            path[pathSize - 1] == path[pathSize - 2] || path[pathSize - 1] == endPoint) {
            goto doStep;
        }
        precalcLength[pathSize - 1] = precalcLength[pathSize - 2] + discreteIntegral(grid, path[pathSize - 2], path[pathSize - 1]);
        if (precalcLength[pathSize - 1] < res) {
            if (pathSize < PATHSIZE) {
                linesByPoints(coeffs, path[pathSize - 2], path[pathSize - 1], sinOfMaxAngle, cosOfMaxAngle);
                bounders[pathSize].first = sf::Vector2i(0, 0);
                bounders[pathSize].second = gridSize;
                if (pathSize <= cosActuality) {
                    subrectBy2Lines(bounders[pathSize], coeffs.first, coeffs.second, path[pathSize - 1]);
                } else {
                    m2 = (sf::Vector2f)(path[pathSize - 1] + endPoint) / 2.f;
                    perp = (sf::Vector2f)perpendicular(endPoint - path[pathSize - 1]) / tans[pathSize];
                    m1 = m2 + perp;
                    m2 = m2 - perp;
                    radius = distanseToEnd[path[pathSize - 1].x][path[pathSize - 1].y] / sins[pathSize];
                    rectBy2CircleAnd2Lines(bounders[pathSize], coeffs.first, coeffs.second, path[pathSize - 1], endPoint, m1, m2, radius, Lcomp[pathSize], Rcomp[pathSize]);
                }
                pathSize++;
                path[pathSize - 1] = bounders[pathSize - 1].first;
                continue;
            }
            curRes = precalcLength[pathSize - 1] + precalcIntegralsToEnd[path[pathSize - 1].x][path[pathSize - 1].y];
            if (curRes < res) {
                res = curRes;
                for (int i = 1; i < pathSize; i++) {
                    circles[i]->setPosition((sf::Vector2f)path[i] * cellSize + sf::Vector2f(cellSize / 2.f, cellSize / 2.f));
                }
            }
        }
        doStep:
        if (path[pathSize - 1].x < bounders[pathSize - 1].second.x - 1) {
            path[pathSize - 1].x++;
        } else {
            path[pathSize - 1].y++;
            path[pathSize - 1].x = bounders[pathSize - 1].first.x;
        }
        if (path[pathSize - 1].y >= bounders[pathSize - 1].second.y) {
            pathSize--;
            if (pathSize > 1) goto doStep;
            else break;
        }
    }
    // std::cout << "counter = " << counter << std::endl;

    diff = std::chrono::high_resolution_clock::now() - start;
    std::cout << "Optimal solution found by Method 1 in " << diff.count() << " seconds.\n";
    findOptimal1->text.setFillColor(sf::Color::Black);
    thr.terminate();
}

void App::calculateOptimalSolution() {
    if (circles.size() < 3) return;
    findOptimalButton->text.setFillColor(sf::Color::Red);
    const auto start = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> diff;

    sf::Vector2i startPoint(circles.front()->getPosition() / cellSize), endPoint(circles.back()->getPosition() / cellSize), corner(0, 0), leftSize(0, 0);

    std::vector<std::vector<float>> precalcIntegralsToEnd(gridSize.x, std::vector<float>(gridSize.y, -1.f)), distanseToEnd(gridSize.x, std::vector<float>(gridSize.y, -1.f));
    for (sf::Vector2i v = sf::Vector2i(0, 0); v.x < gridSize.x; v.x++)
        for (v.y = 0; v.y < gridSize.y; v.y++) {
            precalcIntegralsToEnd[v.x][v.y] = discreteIntegral(grid, v, endPoint);
            distanseToEnd[v.x][v.y] = distanse(v, endPoint);
        }

    float maxAngle = phi_0 * M_PI / 180, cosOfMaxAngle = std::cos(maxAngle), sinOfMaxAngle = std::sin(maxAngle);
    int PATHSIZE = circles.size() - 1;
    std::vector<sf::Vector2i> path(PATHSIZE + 1); path[0] = startPoint;  path[PATHSIZE] = endPoint;
    std::vector<sf::Vector2i> bounders(PATHSIZE);
    std::vector<sf::Vector2f> m1(PATHSIZE), m2(PATHSIZE), perp(PATHSIZE), curM(PATHSIZE);
    std::vector<float> mD(PATHSIZE), mU(PATHSIZE), mL(PATHSIZE), mR(PATHSIZE);
    std::vector<float> radius(PATHSIZE);
    int phiActuality = PATHSIZE - 1;
    while (phiActuality > 0 && (PATHSIZE - phiActuality) * phi_0 < 180.f) phiActuality--;
    std::vector<float> tans(PATHSIZE), sins(PATHSIZE), phis(PATHSIZE);
    for (int i = 1; i < PATHSIZE; i++) {
        tans[i] = 2.f * std::tan((PATHSIZE - i) * maxAngle);
        sins[i] = 2.f * std::sin((PATHSIZE - i) * maxAngle);
        phis[i] = (PATHSIZE - i) * phi_0;
    }
    std::vector<std::vector<bool>> enableCells(gridSize.x, std::vector<bool>(gridSize.y, phiActuality != 0));
    if (phiActuality == 0) {
        enableCells[startPoint.x][startPoint.y] = true;
        float coss = std::cos(maxAngle * (PATHSIZE - 1));
        for (sf::Vector2i v = sf::Vector2i(0, 0); v.x < gridSize.x; v.x++)
            for (v.y = 0; v.y < gridSize.y; v.y++)
                enableCells[v.x][v.y] = normDot(endPoint - v, v - startPoint) >= coss;
        enableCells[endPoint.x][endPoint.y] = true;
    }
    std::vector<std::vector<float>> precalcOptimalLengthToEnd = optimalPathLength(grid, endPoint, enableCells);
    int pathSize = 1;
    std::vector<float> precalcLength(PATHSIZE, 0.f);
    float res = discreteIntegral(grid, startPoint, endPoint), curRes = 0.f;
    std::vector<std::pair<LineCoeffs, LineCoeffs>> coeffs(PATHSIZE);
    std::vector<LineCoeffs> mainCoeffs(PATHSIZE); lineByPoints(mainCoeffs[1], path[0], endPoint);
    std::vector<bool> firstSideDone(PATHSIZE, false);

    if (pathSize <= phiActuality) {
        pathSize++;
        path[pathSize - 1] = sf::Vector2i(0, 0);
        bounders[pathSize - 1] = gridSize - 1;
        firstSideDone[pathSize - 1] = true;
    } else {
        m2[pathSize] = (sf::Vector2f)(path[pathSize - 1] + endPoint) / 2.f;
        perp[pathSize] = (sf::Vector2f)perpendicular(endPoint - path[pathSize - 1]) / tans[pathSize];
        m1[pathSize] = m2[pathSize] + perp[pathSize];
        m2[pathSize] = m2[pathSize] - perp[pathSize];
        curM[pathSize] = m1[pathSize];
        radius[pathSize] = distanseToEnd[path[pathSize - 1].x][path[pathSize - 1].y] / sins[pathSize];
        mD[pathSize] = curM[pathSize].y - radius[pathSize]; mU[pathSize] = curM[pathSize].y + radius[pathSize];
        mL[pathSize] = curM[pathSize].x - radius[pathSize]; mR[pathSize] = curM[pathSize].x + radius[pathSize];
        pathSize++;
        path[pathSize - 1].y = std::max(bottomBounder(mD[pathSize - 1], mainCoeffs[pathSize - 1]), 0);
        bounders[pathSize - 1].y = std::min(topBounder(mU[pathSize - 1], mainCoeffs[pathSize - 1]), gridSize.y - 1);
        path[pathSize - 1].x = leftBounder(path[pathSize - 1].y, mainCoeffs[pathSize - 1], curM[pathSize - 1], radius[pathSize - 1]);
        bounders[pathSize - 1].x = rightBounder(path[pathSize - 1].y, mainCoeffs[pathSize - 1], curM[pathSize - 1], radius[pathSize - 1]);
        path[pathSize - 1].x = std::max(path[pathSize - 1].x, 0);
        bounders[pathSize - 1].x = std::min(bounders[pathSize - 1].x, gridSize.x - 1);
    }
    /* best score for grid 150x150, angle 60 degree
    Optimal solution found in 1.18998 seconds.
    start: ( 32,  22)
    turn1: ( 96,  20) 46.78
    turn2: (125,  49) 47.64
    end:   (122, 114)
    path difficulty: 93.635

    best score for grid 200x200, angle 60 degree
    Optimal solution found in 10.9781 seconds.
    start: ( 32,  22)
    turn1: (112,  17) 31.75
    turn2: (168,  47) 59.51
    end:   (173, 171)
    path difficulty: 115.086
    */
    sf::Thread thr([&]{
        while (true) {
            diff = std::chrono::high_resolution_clock::now() - start;
            workInfo->setString("Time: " + floatToString(diff.count(), 2) + " sec\nRemains: " + std::to_string(1 - firstSideDone[1]) + " sides, " + std::to_string(bounders[1].y - path[1].y + 1) + " levels");
        }
    });
    thr.launch();
    while (true) {
        if (path[pathSize - 1].x > bounders[pathSize - 1].x || path[pathSize - 1].y > bounders[pathSize - 1].y ||
            path[pathSize - 1] == path[pathSize - 2] || path[pathSize - 1] == endPoint) {
            goto doStep;
        }
        precalcLength[pathSize - 1] = precalcLength[pathSize - 2] + discreteIntegral(grid, path[pathSize - 2], path[pathSize - 1]);
        if (precalcLength[pathSize - 1] + precalcOptimalLengthToEnd[path[pathSize - 1].x][path[pathSize - 1].y] < res) {
            if (pathSize < PATHSIZE) {
                linesByPoints(coeffs[pathSize], path[pathSize - 2], path[pathSize - 1], sinOfMaxAngle, cosOfMaxAngle);
                path[pathSize] = sf::Vector2i(0, 0);
                bounders[pathSize] = gridSize - 1;
                if (pathSize <= phiActuality) {
                    subrectBy2Lines(path[pathSize - 1].y, bounders[pathSize].y, coeffs[pathSize].first, coeffs[pathSize].second, path[pathSize - 1], gridSize.x);
                    pathSize++;
                    path[pathSize - 1].x = std::max(path[pathSize - 1].x, sectorLeftBounder(path[pathSize - 1].y, coeffs[pathSize - 1].first, coeffs[pathSize - 1].second, 0));
                    bounders[pathSize - 1].x = std::min(bounders[pathSize - 1].x, sectorRightBounder(path[pathSize - 1].y, coeffs[pathSize - 1].first, coeffs[pathSize - 1].second, gridSize.x - 1));
                    firstSideDone[pathSize - 1] = true;
                } else {
                    m2[pathSize] = (sf::Vector2f)(path[pathSize - 1] + endPoint) / 2.f;
                    perp[pathSize] = (sf::Vector2f)perpendicular(endPoint - path[pathSize - 1]) / tans[pathSize];
                    m1[pathSize] = m2[pathSize] + perp[pathSize];
                    m2[pathSize] = m2[pathSize] - perp[pathSize];
                    curM[pathSize] = m1[pathSize];
                    radius[pathSize] = distanseToEnd[path[pathSize - 1].x][path[pathSize - 1].y] / sins[pathSize];
                    mD[pathSize] = curM[pathSize].y - radius[pathSize]; mU[pathSize] = curM[pathSize].y + radius[pathSize];
                    mL[pathSize] = curM[pathSize].x - radius[pathSize]; mR[pathSize] = curM[pathSize].x + radius[pathSize];
                    lineByPoints(mainCoeffs[pathSize], path[pathSize - 2], endPoint);
                    pathSize++;
                    path[pathSize - 1].y = std::max(bottomBounder(mD[pathSize - 1], mainCoeffs[pathSize - 1]), 0);
                    bounders[pathSize - 1].y = std::min(topBounder(mU[pathSize - 1], mainCoeffs[pathSize - 1]), gridSize.y - 1);
                    yBounderByCircleAnd2Lines(path[pathSize - 1].y, bounders[pathSize - 1].y, coeffs[pathSize - 1].first, coeffs[pathSize - 1].second, path[pathSize - 2], curM[pathSize - 1], radius[pathSize - 1]);
                    if (phis[pathSize - 1] < 90.f) {
                        path[pathSize - 1].y     = std::max(path[pathSize - 1].y,     (int)(std::max(m1[pathSize - 1].x, m2[pathSize - 1].x) > std::max(path[pathSize - 2].x, endPoint.x) ? std::min(path[pathSize - 2].y, endPoint.y) : std::max(m1[pathSize - 1].y, m2[pathSize - 1].y) - radius[pathSize - 1]));
                        bounders[pathSize - 1].y = std::min(bounders[pathSize - 1].y, (int)(std::max(m1[pathSize - 1].x, m2[pathSize - 1].x) > std::max(path[pathSize - 2].x, endPoint.x) ? std::max(path[pathSize - 2].y, endPoint.y) : std::min(m1[pathSize - 1].y, m2[pathSize - 1].y) + radius[pathSize - 1]));
                    }
                    path[pathSize - 1].x = std::max(leftBounder(path[pathSize - 1].y, mainCoeffs[pathSize - 1], curM[pathSize - 1], radius[pathSize - 1]),
                                                    sectorLeftBounder(path[pathSize - 1].y, coeffs[pathSize - 1].first, coeffs[pathSize - 1].second, 0));
                    bounders[pathSize - 1].x = std::min(rightBounder(path[pathSize - 1].y, mainCoeffs[pathSize - 1], curM[pathSize - 1], radius[pathSize - 1]),
                                                       sectorRightBounder(path[pathSize - 1].y, coeffs[pathSize - 1].first, coeffs[pathSize - 1].second, gridSize.x - 1));

                    firstSideDone[pathSize - 1] = false;
                }
                path[pathSize - 1].x = std::max(path[pathSize - 1].x, 0);
                bounders[pathSize - 1].x = std::min(bounders[pathSize - 1].x, gridSize.x - 1);
                continue;
            }
            curRes = precalcLength[pathSize - 1] + precalcIntegralsToEnd[path[pathSize - 1].x][path[pathSize - 1].y];
            if (curRes < res) {
                res = curRes;
                for (int i = 1; i < pathSize; i++) {
                    circles[i]->setPosition((sf::Vector2f)path[i] * cellSize + sf::Vector2f(cellSize / 2.f, cellSize / 2.f));
                }
            }
        }
        doStep:
        if (path[pathSize - 1].x <= bounders[pathSize - 1].x) {
            path[pathSize - 1].x++;
        } else {
            path[pathSize - 1].y++;
            if (pathSize - 1 <= phiActuality) {
                path[pathSize - 1].x = 0;
                bounders[pathSize - 1].x = gridSize.x - 1;
            } else {
                path[pathSize - 1].x = leftBounder(path[pathSize - 1].y, mainCoeffs[pathSize - 1], curM[pathSize - 1], radius[pathSize - 1]);
                bounders[pathSize - 1].x = rightBounder(path[pathSize - 1].y, mainCoeffs[pathSize - 1], curM[pathSize - 1], radius[pathSize - 1]);
            }
            if (pathSize > 2) {
                path[pathSize - 1].x = std::max(path[pathSize - 1].x, sectorLeftBounder(path[pathSize - 1].y, coeffs[pathSize - 1].first, coeffs[pathSize - 1].second, 0));
                bounders[pathSize - 1].x = std::min(bounders[pathSize - 1].x, sectorRightBounder(path[pathSize - 1].y, coeffs[pathSize - 1].first, coeffs[pathSize - 1].second, gridSize.x - 1));
            }
            path[pathSize - 1].x = std::max(path[pathSize - 1].x, 0);
            bounders[pathSize - 1].x = std::min(bounders[pathSize - 1].x, gridSize.x - 1);
        }
        if (path[pathSize - 1].y > bounders[pathSize - 1].y) {
            if (!firstSideDone[pathSize - 1]) {
                firstSideDone[pathSize - 1] = true;
                mainCoeffs[pathSize - 1] *= -1.f;
                curM[pathSize - 1] = m2[pathSize - 1];
                mD[pathSize - 1] = curM[pathSize - 1].y - radius[pathSize - 1]; mU[pathSize - 1] = curM[pathSize - 1].y + radius[pathSize - 1];
                mL[pathSize - 1] = curM[pathSize - 1].x - radius[pathSize - 1]; mR[pathSize - 1] = curM[pathSize - 1].x + radius[pathSize - 1];
                path[pathSize - 1].y = std::max(bottomBounder(mD[pathSize - 1], mainCoeffs[pathSize - 1]), 0);
                bounders[pathSize - 1].y = std::min(topBounder(mU[pathSize - 1], mainCoeffs[pathSize - 1]), gridSize.y - 1);
                if (pathSize > 2) {
                    yBounderByCircleAnd2Lines(path[pathSize - 1].y, bounders[pathSize - 1].y, coeffs[pathSize - 1].first, coeffs[pathSize - 1].second, path[pathSize - 2], curM[pathSize - 1], radius[pathSize - 1]);
                    if (phis[pathSize - 1] < 90.f) {
                        path[pathSize - 1].y     = std::max(path[pathSize - 1].y,     (int)(std::max(m1[pathSize - 1].x, m2[pathSize - 1].x) > std::max(path[pathSize - 2].x, endPoint.x) ? std::min(path[pathSize - 2].y, endPoint.y) : std::max(m1[pathSize - 1].y, m2[pathSize - 1].y) - radius[pathSize - 1]));
                        bounders[pathSize - 1].y = std::min(bounders[pathSize - 1].y, (int)(std::max(m1[pathSize - 1].x, m2[pathSize - 1].x) > std::max(path[pathSize - 2].x, endPoint.x) ? std::max(path[pathSize - 2].y, endPoint.y) : std::min(m1[pathSize - 1].y, m2[pathSize - 1].y) + radius[pathSize - 1]));
                    }
                    path[pathSize - 1].x = sectorLeftBounder(path[pathSize - 1].y, coeffs[pathSize - 1].first, coeffs[pathSize - 1].second, 0);
                    bounders[pathSize - 1].x = sectorRightBounder(path[pathSize - 1].y, coeffs[pathSize - 1].first, coeffs[pathSize - 1].second, gridSize.x - 1);
                    path[pathSize - 1].x = std::max(path[pathSize - 1].x, leftBounder(path[pathSize - 1].y, mainCoeffs[pathSize - 1], curM[pathSize - 1], radius[pathSize - 1]));
                    bounders[pathSize - 1].x = std::min(bounders[pathSize - 1].x, rightBounder(path[pathSize - 1].y, mainCoeffs[pathSize - 1], curM[pathSize - 1], radius[pathSize - 1]));
                } else {
                    path[pathSize - 1].x = leftBounder(path[pathSize - 1].y, mainCoeffs[pathSize - 1], curM[pathSize - 1], radius[pathSize - 1]);
                    bounders[pathSize - 1].x = rightBounder(path[pathSize - 1].y, mainCoeffs[pathSize - 1], curM[pathSize - 1], radius[pathSize - 1]);
                }
                path[pathSize - 1].x = std::max(path[pathSize - 1].x, 0);
                bounders[pathSize - 1].x = std::min(bounders[pathSize - 1].x, gridSize.x - 1);
            } else {
                pathSize--;
                if (pathSize > 1) goto doStep;
                else break;
            }
        }
    }

    diff = std::chrono::high_resolution_clock::now() - start;
    std::cout << "Optimal solution found by Method 2 in " << diff.count() << " seconds.\n";
    findOptimalButton->text.setFillColor(sf::Color::Black);
    thr.terminate();
}

void App::calculateOptimalSolutionByAngle() {
    if (circles.size() < 3) return;
    findOptimal0->text.setFillColor(sf::Color::Red);
    const auto start = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> diff;

    sf::Vector2i startPoint(circles.front()->getPosition() / cellSize), endPoint(circles.back()->getPosition() / cellSize), corner(0, 0), leftSize(0, 0);

    std::vector<std::vector<float>> precalcIntegralsToEnd(gridSize.x, std::vector<float>(gridSize.y, -1.f));
    for (sf::Vector2i v = sf::Vector2i(0, 0); v.x < gridSize.x; v.x++)
        for (v.y = 0; v.y < gridSize.y; v.y++)
            precalcIntegralsToEnd[v.x][v.y] = discreteIntegral(grid, v, endPoint);

    std::vector<std::vector<float>> precalcAngles(gridSize.x * 2, std::vector<float>(gridSize.y * 2, 0.f));
    for (int x = 0; x < gridSize.x; x++)
        for (int y = 0; y < gridSize.y; y++) {
            if (x == 0 && y == 0) continue;
            precalcAngles[gridSize.x + x][gridSize.y + y] = std::atan2(x, y);
            precalcAngles[gridSize.x - x][gridSize.y + y] =   M_PI - precalcAngles[gridSize.x + x][gridSize.y + y];
            precalcAngles[gridSize.x + x][gridSize.y - y] =        - precalcAngles[gridSize.x + x][gridSize.y + y];
            precalcAngles[gridSize.x - x][gridSize.y - y] = - M_PI + precalcAngles[gridSize.x + x][gridSize.y + y];
        }

    int PATHSIZE = circles.size() - 1;
    std::vector<sf::Vector2i> path(PATHSIZE);
    path[0] = startPoint;
    path[1] = corner;
    int pathSize = 2;
    std::vector<float> precalcLength(PATHSIZE, 0.f);
    float res = discreteIntegral(grid, startPoint, endPoint);
    float curRes = 0.f;
    bool needToStep = false;
    std::vector<float> angles(PATHSIZE, 0.f);
    angles[pathSize - 1] = precalcAngles[gridSize.x + path[pathSize - 1].x - path[pathSize - 2].x][gridSize.y + path[pathSize - 1].y - path[pathSize - 2].y];
    float maxAngle = phi_0 * M_PI / 180;
    double difference, pi2 = 2 * M_PI;
    /* best score for grid 150x150, angle 60 degree
    Optimal solution found in 21.6836 seconds.
    start: (32, 22)
    turn1: (98, 20)
    turn2: (125, 49)
    end:   (122, 114)
    path difficulty: 93.635
    */
    sf::Thread thr([&]{
        while (true) {
            diff = std::chrono::high_resolution_clock::now() - start;
            workInfo->setString("Time: " + floatToString(diff.count(), 2) + " sec\nDone: " + floatToString(100 * float(path[1].x + path[1].y * gridSize.y) / gridSize.x / gridSize.y, 2) + " %");
        }
    });
    thr.launch();
    while (pathSize > 1) {
        if (!needToStep) {
            precalcLength[pathSize - 1] = precalcLength[pathSize - 2] + discreteIntegral(grid, path[pathSize - 2], path[pathSize - 1]);
            if (precalcLength[pathSize - 1] < res) {
                if (pathSize < PATHSIZE) {
                    pathSize++;
                    path[pathSize - 1] = corner;
                    needToStep = path[pathSize - 1] == path[pathSize - 2];
                    angles[pathSize - 1] = precalcAngles[gridSize.x + path[pathSize - 1].x - path[pathSize - 2].x][gridSize.y + path[pathSize - 1].y - path[pathSize - 2].y];
                    continue;
                }
                curRes = precalcLength[pathSize - 1] + precalcIntegralsToEnd[path[pathSize - 1].x][path[pathSize - 1].y];
                if (curRes < res) {
                    res = curRes;
                    for (int i = 1; i < pathSize; i++) {
                        circles[i]->setPosition((sf::Vector2f)path[i] * cellSize + sf::Vector2f(cellSize / 2.f, cellSize / 2.f));
                    }
                }
            }
        }
        needToStep = false;
        if (path[pathSize - 1].x < gridSize.x - 1) {
            path[pathSize - 1].x++;
        } else {
            path[pathSize - 1].y++;
            path[pathSize - 1].x = 0;
        }
        if (path[pathSize - 1].y == gridSize.y) {
            pathSize--;
            needToStep = true;
            continue;
        }
        needToStep = path[pathSize - 1] == path[pathSize - 2] || path[pathSize - 1] == endPoint;
        if (!needToStep) {
            angles[pathSize - 1] = precalcAngles[gridSize.x + path[pathSize - 1].x - path[pathSize - 2].x][gridSize.y + path[pathSize - 1].y - path[pathSize - 2].y];
            if (pathSize > 2) {
                difference = std::abs(angles[pathSize - 1] - angles[pathSize - 2]);
                needToStep = difference > maxAngle && pi2 - difference > maxAngle;
            }
            if (!needToStep) {
                difference = std::abs(precalcAngles[gridSize.x + endPoint.x - path[pathSize - 1].x][gridSize.y + endPoint.y - path[pathSize - 1].y] - angles[pathSize - 1]);
                needToStep = difference > maxAngle * (PATHSIZE - pathSize + 1) && pi2 - difference > maxAngle * (PATHSIZE - pathSize + 1);
            }
        }
    }

    const auto end = std::chrono::high_resolution_clock::now();
    diff = end - start;
    std::cout << "Optimal solution found by Method 2 in " << diff.count() << " seconds.\n";
    findOptimal0->text.setFillColor(sf::Color::Black);
    thr.terminate();
}

void App::makeExperiment() {
    std::vector<float> times(90, 0.f);
    int amount = 1;
    // std::ofstream file("experiment.txt");
    for (int i = phi_0; i < 90; i++) {
        phi_0 = i + 1;
        for (int j = 0; j < amount; j++) {
            const auto start = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> diff;
            calculateOptimalSolution();
            diff = std::chrono::high_resolution_clock::now() - start;
            times[i] += diff.count();
        }
        std::cout << phi_0 << ", " << times[i] / amount << '\n';
    }
    // file.close();
}