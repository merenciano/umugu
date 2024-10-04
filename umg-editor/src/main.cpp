#include "umugu_editor.h"

int main()
{
    umugu_editor::Init();
    while (!umugu_editor::PollEvents())
    {
        umugu_editor::Render();
    }
    umugu_editor::Close();
    return 0;
}
