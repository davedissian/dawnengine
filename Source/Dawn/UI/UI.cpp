/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2015 (avedissian.david@gmail.com)
 */
#include "Common.h"
#include "Input/Input.h"
#include "Renderer/Renderer.h"
#include "UI.h"

#include "RocketInterface.h"
#include "ImGuiInterface.h"

NAMESPACE_BEGIN

#define INTERFACE_RENDER_QUEUE (Ogre::RENDER_QUEUE_OVERLAY)

const EventType EvtData_UIClick::eventType(0xe135dd7);
const EventType EvtData_UISubmit::eventType(0x3d02cddc);

UI::UI(Renderer* rs, Input* im, LuaState* ls) : mRenderSystem(rs)
{
    // Force the interface render queue to be created
    rs->GetSceneMgr()->getRenderQueue()->getQueueGroup(INTERFACE_RENDER_QUEUE);

    // Add render queue Listener
    rs->GetSceneMgr()->addRenderQueueListener(this);

    // Set up interfaces
    mRocketInterface = MakeShared<RocketInterface>(rs);
    Rocket::Core::SetRenderInterface(mRocketInterface.get());
    Rocket::Core::SetSystemInterface(mRocketInterface.get());
    Rocket::Core::SetFileInterface(mRocketInterface.get());

    // Initialise libRocket
    Rocket::Core::Initialise();
    Rocket::Controls::Initialise();
    mContext = Rocket::Core::CreateContext("default",
            Rocket::Core::Vector2i(rs->GetWidth(), rs->GetHeight()));
    Rocket::Debugger::Initialise(mContext);

    // Load fonts
    Rocket::Core::FontDatabase::LoadFontFace(
            "LiberationSans-Regular.ttf", "Liberation", Rocket::Core::Font::STYLE_NORMAL,
            Rocket::Core::Font::WEIGHT_NORMAL);

    // Initialise ImGui
    mImGuiInterface = MakeShared<ImGuiInterface>(rs, im);

    // Set up the console
    mConsole = MakeShared<Console>(this, ls);

    // Event Delegates
    ADD_LISTENER(UI, EvtData_TextInput);
    ADD_LISTENER(UI, EvtData_KeyDown);
    ADD_LISTENER(UI, EvtData_KeyUp);
    ADD_LISTENER(UI, EvtData_MouseDown);
    ADD_LISTENER(UI, EvtData_MouseUp);
    ADD_LISTENER(UI, EvtData_MouseMove);
    ADD_LISTENER(UI, EvtData_MouseWheel);
}

UI::~UI()
{
    REMOVE_LISTENER(UI, EvtData_TextInput);
    REMOVE_LISTENER(UI, EvtData_KeyDown);
    REMOVE_LISTENER(UI, EvtData_KeyUp);
    REMOVE_LISTENER(UI, EvtData_MouseDown);
    REMOVE_LISTENER(UI, EvtData_MouseUp);
    REMOVE_LISTENER(UI, EvtData_MouseMove);
    REMOVE_LISTENER(UI, EvtData_MouseWheel);

    mConsole.reset();

    // Shut down libRocket
    mContext->RemoveReference();
    Rocket::Core::Shutdown();

    // Release interfaces
    mImGuiInterface.reset();
    mRocketInterface.reset();

    mRenderSystem->GetSceneMgr()->removeRenderQueueListener(this);
}

void UI::BeginFrame()
{
    mImGuiInterface->BeginFrame();
}

void UI::Update(float dt)
{
}

void UI::PreRender()
{
}

Layout* UI::LoadLayout(const String& filename)
{
    // TODO: What if this fails?
    auto fn = Rocket::Core::String(filename.c_str());
    Rocket::Core::ElementDocument* document = mContext->LoadDocument(fn);

    // Set the title
    Rocket::Core::Element* title = document->GetElementById("title");
    if (title)
        title->SetInnerRML(document->GetTitle());

    return new Layout(this, document);
}

void UI::UnloadLayout(Layout* layout)
{
    delete layout;
}

void UI::HandleEvent(EventDataPtr eventData)
{
    using Rocket::Core::Input::KeyIdentifier;

    if (EventIs<EvtData_TextInput>(eventData))
    {
        auto castedEventData = StaticPointerCast<EvtData_TextInput>(eventData);

        for (auto c : castedEventData->text)
            mImGuiInterface->OnTextInput(c);

        // Send the input to libRocket
        if (castedEventData->text != "`")
            mContext->ProcessTextInput(Rocket::Core::String(castedEventData->text.c_str()));
    }

    if (EventIs<EvtData_KeyDown>(eventData))
    {
        auto castedEventData = StaticPointerCast<EvtData_KeyDown>(eventData);

        // Toggle the console
        if (castedEventData->keycode == SDLK_BACKQUOTE || castedEventData->keycode == SDLK_F12)
        {
            if (castedEventData->mod & KMOD_SHIFT)
            {
                Rocket::Debugger::SetVisible(!Rocket::Debugger::IsVisible());
            }
            else
            {
                mConsole->SetVisible(!mConsole->IsVisible());
            }
            return;
        }

        mImGuiInterface->OnKey(castedEventData->keycode, true, castedEventData->mod);

        // Send the key to libRocket
        int key = mRocketInterface->MapSDLKeyCode(castedEventData->keycode);
        mContext->ProcessKeyDown(KeyIdentifier(key),
                                 mRocketInterface->MapSDLKeyMod(castedEventData->mod));
    }

    if (EventIs<EvtData_KeyUp>(eventData))
    {
        auto castedEventData = StaticPointerCast<EvtData_KeyUp>(eventData);
        mImGuiInterface->OnKey(castedEventData->keycode, false, castedEventData->mod);
        int key = mRocketInterface->MapSDLKeyCode(castedEventData->keycode);
        mContext->ProcessKeyUp(KeyIdentifier(key),
                               mRocketInterface->MapSDLKeyMod(castedEventData->mod));
    }

    if (EventIs<EvtData_MouseDown>(eventData))
    {
        auto castedEventData = StaticPointerCast<EvtData_MouseDown>(eventData);
        mImGuiInterface->OnMouseButton(castedEventData->button);
        mContext->ProcessMouseButtonDown(mRocketInterface->MapSDLMouseButton(castedEventData->button),
                                         mRocketInterface->MapSDLKeyMod(SDL_GetModState()));
    }

    if (EventIs<EvtData_MouseUp>(eventData))
    {
        auto castedEventData = StaticPointerCast<EvtData_MouseUp>(eventData);
        mContext->ProcessMouseButtonUp(mRocketInterface->MapSDLMouseButton(castedEventData->button),
                                       mRocketInterface->MapSDLKeyMod(SDL_GetModState()));
    }

    if (EventIs<EvtData_MouseMove>(eventData))
    {
        auto castedEventData = StaticPointerCast<EvtData_MouseMove>(eventData);
        mContext->ProcessMouseMove(castedEventData->pos.x, castedEventData->pos.y,
                                   mRocketInterface->MapSDLKeyMod(SDL_GetModState()));
    }

    if (EventIs<EvtData_MouseWheel>(eventData))
    {
        auto castedEventData = StaticPointerCast<EvtData_MouseWheel>(eventData);
        mImGuiInterface->OnMouseScroll(castedEventData->motion.y);
        mContext->ProcessMouseWheel(-castedEventData->motion.y,
                                    mRocketInterface->MapSDLKeyMod(SDL_GetModState()));
    }
}

void UI::ProcessEvent(Rocket::Core::Event& event)
{
    String type = event.GetType().CString();
    String id = event.GetCurrentElement()->GetId().CString();

    Map<String, String> parameters;

    // Log event details
    std::stringstream out;
    out << "[librocket event] type: " << type << ", id: " << id << ", params: (";
    int i = 0;
    Rocket::Core::String key;
    Rocket::Core::String value;
    int totalLength = out.str().length();
    while (event.GetParameters()->Iterate(i, key, value))
    {
        int thisLength = key.Length() + value.Length() + 2;
        if (totalLength + thisLength > 120)
        {
            out << std::endl << "[librocket event]   ";
            totalLength = thisLength + 20;
        }
        else
        {
            totalLength += thisLength;
        }

        // Print parameters
        out << key.CString() << "=" << value.CString() << ",";
        parameters.insert(MakePair<String, String>(key.CString(), value.CString()));
    }
    out << ")";
    LOG << out.str();

    if (type == "click")
        EventSystem::inst().QueueEvent(MakeShared<EvtData_UIClick>(id, parameters));
    if (type == "submit")
        EventSystem::inst().QueueEvent(MakeShared<EvtData_UISubmit>(id, parameters));
}

void UI::renderQueueStarted(Ogre::uint8 queueGroupId, const Ogre::String&, bool&)
{
    if (queueGroupId == INTERFACE_RENDER_QUEUE &&
        Ogre::Root::getSingleton().getRenderSystem()->_getViewport()->getOverlaysEnabled())
    {
        mContext->Update();
        ConfigureRenderSystem();
        mContext->Render();
        ImGui::Render();
    }
}

void UI::ConfigureRenderSystem()
{
    Ogre::RenderSystem* renderSystem = mRenderSystem->GetOgreRenderSystem();

    // Set up the projection and view matrices
    Ogre::Matrix4 projectionMatrix;
    BuildProjectionMatrix(projectionMatrix);
    renderSystem->_setProjectionMatrix(projectionMatrix);
    renderSystem->_setViewMatrix(Ogre::Matrix4::IDENTITY);

    // Disable lighting, as all of Rocket's geometry is unlit
    renderSystem->setLightingEnabled(false);

    // Disable depth-buffering; all of the geometry is already depth-sorted
    renderSystem->_setDepthBufferParams(false, false);

    // Disable culling
    renderSystem->_setCullingMode(Ogre::CULL_NONE);

    // Disable fogging
    renderSystem->_setFog(Ogre::FOG_NONE);

    // Enable writing to all four channels
    renderSystem->_setColourBufferWriteEnabled(true, true, true, true);

    // Unbind any vertex or fragment programs bound previously by the application
    renderSystem->unbindGpuProgram(Ogre::GPT_FRAGMENT_PROGRAM);
    renderSystem->unbindGpuProgram(Ogre::GPT_VERTEX_PROGRAM);

    // Set texture settings to clamp along both axes
    Ogre::TextureUnitState::UVWAddressingMode addressingMode;
    addressingMode.u = Ogre::TextureUnitState::TAM_CLAMP;
    addressingMode.v = Ogre::TextureUnitState::TAM_CLAMP;
    addressingMode.w = Ogre::TextureUnitState::TAM_CLAMP;
    renderSystem->_setTextureAddressingMode(0, addressingMode);

    // Set the texture coordinates for unit 0 to be read from unit 0
    renderSystem->_setTextureCoordSet(0, 0);

    // Disable texture coordinate calculation
    renderSystem->_setTextureCoordCalculation(0, Ogre::TEXCALC_NONE);

    // Enable linear filtering; images should be rendering 1 texel == 1 pixel, so point filtering
    // could be used except in the case of scaling tiled decorators
    renderSystem->_setTextureUnitFiltering(0, Ogre::FO_LINEAR, Ogre::FO_LINEAR, Ogre::FO_POINT);

    // Disable texture coordinate transforms
    renderSystem->_setTextureMatrix(0, Ogre::Matrix4::IDENTITY);

    // Reject pixels with an alpha of 0
    renderSystem->_setAlphaRejectSettings(Ogre::CMPF_GREATER, 0, false);

    // Disable all texture units but the first
    renderSystem->_disableTextureUnitsFrom(1);

    // Enable simple alpha blending
    renderSystem->_setSceneBlending(Ogre::SBF_SOURCE_ALPHA, Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);

    // Disable depth bias
    renderSystem->_setDepthBias(0, 0);
}

void UI::BuildProjectionMatrix(Ogre::Matrix4& projectionMatrix)
{
    float zNear = -1.0f;
    float zFar = 1.0f;

    projectionMatrix = Ogre::Matrix4::ZERO;
    projectionMatrix[0][0] = 2.0f / (float)mRenderSystem->GetWidth();
    projectionMatrix[0][3] = -1.0f;
    projectionMatrix[1][1] = -2.0f / (float)mRenderSystem->GetHeight();
    projectionMatrix[1][3] = 1.0f;
    projectionMatrix[2][2] = -2.0f / (zFar - zNear);
    projectionMatrix[3][3] = 1.0f;
}

NAMESPACE_END