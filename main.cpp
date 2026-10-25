#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>
#include <cmath>
#include <nlohmann/json.hpp>

#define PI 3.141592654

const sf::Font font("res/font.ttf");

const float SPEED = 200.f;
const float MIN_DIST_REACHED = 5.f;



struct Player
{
    Player(std::string name, sf::Color col)
    :name(name), spriteColor(col){}
    sf::Vector2f physicalPos;
    int realPos = 0;
    sf::Color spriteColor;
    sf::Vector2f targetPhysical = sf::Vector2f(0, 0);
    bool reached = false;
    int currentPos = 0;
    std::string name;
};

struct CellEffectContext
{
    CellEffectContext(Player& player, int lastDiceRoll, int& gameState, bool& keepTurn)
    :player(player), lastDiceRoll(lastDiceRoll), gameState(gameState), keepTurn(keepTurn)
    {

    }
    Player& player;
    int lastDiceRoll;
    int& gameState;
    bool& keepTurn;
};

struct Cell
{
    std::string effectKey = "None";
    std::function<void(CellEffectContext)> effect = [](CellEffectContext ctx){};
    sf::Color cellColor = sf::Color::White;
};

class Dice6
{
    public:
        Dice6()
        {
            sprite = sf::RectangleShape({size, size});
        }
        int roll()
        {
            lastValue = rand()%6+1;
            return lastValue;
        }
        void draw(sf::RenderWindow& window, sf::Vector2f position, float scale)
        {
            float rad = scale * size * 0.1f;
            circleSprite.setRadius(rad);
            circleSprite.setOrigin({rad, rad});
            circleSprite.setFillColor(sf::Color::Black);
            sprite.setPosition(position);
            sprite.setScale({scale, scale});
            window.draw(sprite);
            for(int i = 0; i < 9; i++)
            {
                if(1<<i & number[lastValue])
                {
                    circleSprite.setPosition(sprite.getPosition()+circleRatios[i+1] * (float)size * scale);
                    window.draw(circleSprite);
                }
            }
        }
        int getLastValue()
        {
            return lastValue;
        }
        static int getSize()
        {
            return size;
        }
    private:
        static const int size = 50;
        const float spacing = 0.2f;
        sf::Vector2f circleRatios[10] = {
            {0, 0},
            {spacing, spacing},
            {0.5, spacing},
            {1.f - spacing, spacing},
            {spacing, 0.5f},
            {0.5f, 0.5f},
            {1.f-spacing, 0.5f},
            {spacing, 1.f-spacing},
            {0.5f, 1.f-spacing},
            {1.f-spacing, 1.f-spacing}
        };
        uint16_t number[7] = {
            0,
            16,
            68,
            84,
            325,
            341,
            365
        };
        sf::CircleShape circleSprite;
        sf::RectangleShape sprite;
        int lastValue = 1;
};

void initBoard(sf::VertexArray& mesh, float scale, sf::Vector2f offset, std::vector<sf::Vector2f>& center, std::vector<Cell>& game);
void drawNumbers(sf::RenderWindow& window, std::vector<sf::Vector2f>& center, float scale, sf::Vector2f offset);
void addSpecialCell(int position, Cell cell, std::vector<Cell>& game, sf::VertexArray& mesh);
void drawPlayer(sf::RenderWindow& window, float scale, float outlineWidth, Player& player);
void setCell(Player& p, int cell, std::vector<sf::Vector2f>& center, float scale);
sf::Vector2f randomVector(float min, float max);

enum STATE
{
    ROLLING_DICE,
    MOVING_PLAYER,
    END
};

int main()
{
    float dt = 1/60.f;
    sf::Clock deltaClock;
    sf::VertexArray mesh(sf::PrimitiveType::Triangles, 0);
    std::vector<sf::Vector2f> center;
    std::vector<Player> players;
    std::vector<Cell> game;
    std::vector<Dice6> dices = {Dice6(), Dice6()};
    float scale = 0.4f;
    sf::Vector2f offset = {200, 512};
    initBoard(mesh, scale, offset, center, game);
    sf::Text infoText(font);
    int turnIndex = 0;
    int gameState = STATE::ROLLING_DICE;
    std::ifstream file("res/local.json");
    if(!file.good())
    {
        std::cout << "couldn't parse json file";
        return -1;
    }
    nlohmann::json data = nlohmann::json::parse(file);
    std::string lang = "en";
    int playerCount = 0;
    std::cout << data[lang]["prompt"] << std::endl;
    std::cin >> playerCount;
    std::vector<sf::Color> colors = {
        sf::Color::Red,
        sf::Color::Blue,
        sf::Color::Green,
        sf::Color::Yellow
    };
    for(int i = 0; i < playerCount; i++)
    {
        std::string name = "";
        std::cout << data[lang]["player_prompt"] << std::endl;
        std::cin >> name;
        Player p(name, colors[i]);
        setCell(p, 0, center, scale);
        p.physicalPos = p.targetPhysical;
        players.push_back(p);

    }
    sf::RenderWindow window(sf::VideoMode({512, 512}), "SFML window");
    window.setFramerateLimit(60);
	while(window.isOpen())
	{
	    while(std::optional<sf::Event> e = window.pollEvent())
		{
		    if(e->is<sf::Event::Closed>())
				window.close();
			if(e->is<sf::Event::KeyPressed>() && e->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Space)
            {
                gameState = STATE::MOVING_PLAYER;
                players[turnIndex].realPos += dices[0].roll() + dices[1].roll();
                setCell(players[turnIndex], players[turnIndex].currentPos+1, center, scale);
            }
		}
		window.clear(sf::Color::Black);
		window.draw(mesh);
		drawNumbers(window, center, scale, offset);
		if(gameState == STATE::ROLLING_DICE)
		{
            for(auto& d : dices) d.roll();
		}

  		for(int i = 0; i < dices.size(); i++)
  		{
            sf::Vector2f c = (center[31] + center[14]) / 2.f;
            sf::Vector2f pos = {(-1 + 2*i) * scale * Dice6::getSize(), 0.0};
            pos += c;
  		    dices[i].draw(window, pos, scale);
  		}
		infoText.setPosition({0.0, 0.0});
		infoText.setFillColor(sf::Color::White);
		std::string s =
		(std::string)data[lang]["player_turn"][0] +
		players[turnIndex].name +
		(std::string)data[lang]["player_turn"][1];
		infoText.setString(s);
		window.draw(infoText);
		for(auto& p: players)
        {
            if(!p.reached)
            {
                sf::Vector2f target = center[p.realPos];
                sf::Vector2f diff = p.targetPhysical - p.physicalPos;
                p.physicalPos += diff.normalized() * SPEED * dt;
                if(diff.length() < MIN_DIST_REACHED)
                {
                    p.reached = true;
                    p.physicalPos = p.targetPhysical;
                    if(p.currentPos < p.realPos)
                    {
                        setCell(p, p.currentPos+1, center, scale);
                    }
                    else
                    {
                        int diceSum = 0;
                        for(auto& d : dices)
                            diceSum += d.getLastValue();
                        bool keepturn = false;
                        game[p.currentPos].effect(CellEffectContext(p, diceSum, turnIndex, keepturn));
                        turnIndex = (turnIndex+1)%playerCount;
                        if(!keepturn)
                        {
                            gameState = STATE::ROLLING_DICE;
                        }
                    }
                }
            }
            drawPlayer(window, scale, 2.f*scale, p);
        }
		window.display();
	}
	return 0;
}

// mesh est un moyen de n'avoir qu'un appel draw pour tout le plateau
// center est un vecteur qui permet de stocker les position centrales des cases pour faciliter le déplacement
// des pions ainsi que déssiner les nombres
void initBoard(sf::VertexArray& mesh, float scale, sf::Vector2f offset, std::vector<sf::Vector2f>& center, std::vector<Cell>& game)
{
    mesh.resize(6 * 62);
    float rot = 0.f;
    float size = 50 * scale;
    sf::Vector2f topleft = offset;
    sf::Vector2f topright = offset + sf::Vector2f(size, 0.f);
    sf::Vector2f bottomleft = offset + sf::Vector2f(0.f, size);
    sf::Vector2f bottomright = offset + sf::Vector2f(size, size);
    float angle = 0.f;
    Cell defaultCell;
    for(int i = 0; i < 62; i++)
    {
        game.push_back(Cell());
        int index = i*6;
        rot = 0.f;
        if(i>5 && i <= 17)
            rot = -PI/12.f;
        if(i>24 && i <= 34)
            rot = -PI/10.f;
        if(i>41 && i <= 49)
            rot = -PI/8.f;
        if(i>55)
        {
            rot = -PI/9.f;
            if(i == 61)
                bottomright.y = topright.y;
        }
        center.push_back((topleft + topright + bottomleft + bottomright)/4.f);
        mesh[index].position = topleft;
        mesh[index+1].position = topright;
        mesh[index+2].position = bottomleft;
        mesh[index+3].position = topright;
        mesh[index+4].position = bottomleft;
        mesh[index+5].position = bottomright;
        topleft = topright;
        bottomleft = bottomright;
        sf::Vector2f angleVec = sf::Vector2f(cos(angle), sin(angle)) * size;
        angle += rot;
        topright += angleVec;
        sf::Vector2f squareAngle = sf::Vector2f(cos(angle+PI/2.f), sin(angle+PI/2.f))*size;
        bottomright = topright + squareAngle;
    }
    Cell goBack = Cell();
    goBack.effectKey = "goBack";
    goBack.effect = [&center, scale](CellEffectContext ctx)
    {
        setCell(ctx.player,(ctx.player.realPos-ctx.lastDiceRoll), center, scale);
        ctx.keepTurn = true;
        ctx.player.realPos = ctx.player.currentPos;
        ctx.gameState = STATE::MOVING_PLAYER;
    };
    goBack.cellColor = sf::Color(250, 50, 50);
    addSpecialCell(5, goBack, game, mesh);
    addSpecialCell(9, goBack, game, mesh);
    addSpecialCell(23, goBack, game, mesh);
    addSpecialCell(27, goBack, game, mesh);
    addSpecialCell(41, goBack, game, mesh);
    addSpecialCell(54, goBack, game, mesh);
    Cell bridge;
    bridge.effectKey = "bridge";
    bridge.effect = [](CellEffectContext ctx)
    {
        ctx.player.realPos = 12;
        ctx.keepTurn = true;
        ctx.gameState = STATE::MOVING_PLAYER;
    };
    bridge.cellColor = sf::Color(200, 255, 200);
    addSpecialCell(1, bridge, game, mesh);
    addSpecialCell(2, bridge, game, mesh);
    addSpecialCell(3, bridge, game, mesh);
    addSpecialCell(4, bridge, game, mesh);
}

void drawNumbers(sf::RenderWindow& window, std::vector<sf::Vector2f>& center, float scale, sf::Vector2f offset)
{
    sf::Text text(font);
    text.setFillColor(sf::Color::Black);
    int textSize = 30 * scale;
    text.setCharacterSize(textSize);
    int i = 1;
    for(sf::Vector2f& p : center)
    {
        text.setString(std::to_string(i++));
        text.setPosition(p - sf::Vector2f(textSize/2.f, textSize/2.f));
        window.draw(text);
    }
}

void addSpecialCell(int position, Cell cell, std::vector<Cell>& game, sf::VertexArray& mesh)
{
    mesh[position*6].color = cell.cellColor;
    mesh[position*6+1].color = cell.cellColor;
    mesh[position*6+2].color = cell.cellColor;
    mesh[position*6+3].color = cell.cellColor;
    mesh[position*6+4].color = cell.cellColor;
    mesh[position*6+5].color = cell.cellColor;
    game[position] = cell;
}

void drawPlayer(sf::RenderWindow& window, float scale, float outlineWidth, Player& player)
{
    sf::CircleShape sprite(15.f * scale);
    sprite.setPosition(player.physicalPos);
    sprite.setOutlineThickness(outlineWidth);
    sprite.setFillColor(player.spriteColor);
    sprite.setOrigin({sprite.getRadius(), sprite.getRadius()});
    window.draw(sprite);
}

sf::Vector2f randomVector(float min, float max)
{
    int sum = max+fabs(min);
    return sf::Vector2f((rand()%sum) - fabs(min), (rand()%sum) - fabs(min));
}

void setCell(Player& p, int cell, std::vector<sf::Vector2f>& center, float scale)
{
    p.currentPos = cell;
    p.targetPhysical = center[cell] + randomVector(-10 * scale, 10 * scale);
    p.reached = false;
}
