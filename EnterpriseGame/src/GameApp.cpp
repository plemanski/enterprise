#include <Enterprise.h>

class Game : public Enterprise::Application
{
public:
    Game()
    {
        
    }
    
    ~Game()
    {
        
    }
    
};

Enterprise::Application* Enterprise::CreateApplication()
{
    return new Game();
};