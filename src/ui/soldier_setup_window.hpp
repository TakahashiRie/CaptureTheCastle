#pragma once

#include <SDL_mixer.h>
#include "common.hpp"
#include "components.hpp"
#include "closebtn.hpp"
#include "ecs/events.hpp"
#include "popupbtn.hpp"
#include "play_instructions.hpp"
#include "popup_background.hpp"

class SetupWindow
{
public:
    void init(vec2 screen_size);

    void destroy();

    void draw(const mat3& projection);

    ButtonActions checkButtonClicks(vec2 mouseloc);

    void checkButtonHovers(vec2 mouseloc);

private:
    PopUpBackground background;
    PopUpButton start_btn;
    PlayInstructions instructions;
    Mix_Chunk* m_click;
};
