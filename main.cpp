#include <SFML/Window/Event.hpp>
#include <iostream>
#include <cstdlib>
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>

int main()
{
    sf::RenderWindow window(sf::VideoMode({512, 512}), "SFML window");
    sf::CircleShape circle(256.f);
	std::cout << "Hello World" << std::endl;
	while(window.isOpen())
	{
	    while(std::optional<sf::Event> e = window.pollEvent())
		{
		    if(e->is<sf::Event::Closed>()) window.close();
		}
		window.clear(sf::Color::Black);
	    window.draw(circle);
		window.display();
	}
	return 0;
}
