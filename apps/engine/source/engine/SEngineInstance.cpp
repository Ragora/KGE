/**
 *  @file SEngineInstance.cpp
 *  @brief Source code associated with the Kiaro::EngineInstance singleton class.
 *
 *  This software is licensed under the Draconic Free License version 1. Please refer
 *  to LICENSE.txt for more information.
 *
 *  @author Draconic Entertainment
 *  @copyright (c) 2014 Draconic Entertainment
 */

#include <irrlicht.h>
#include <lua.hpp>

#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/Irrlicht/Renderer.h>

#include <engine/config.hpp>
#include <engine/SEngineInstance.hpp>
#include <engine/Logging.hpp>
#include <engine/SInputListener.hpp>

#include <network/SClient.hpp>

#include <network/CClient.hpp>
#include <network/SServer.hpp>

#include <support/FTime.hpp>
#include <engine/SScheduler.hpp>

#include <engine/CSceneGraph.hpp>

namespace Kiaro
{
    namespace Engine
    {
        static Kiaro::Engine::SEngineInstance *sInstance = NULL;

        SEngineInstance *SEngineInstance::getPointer(void)
        {
            if (!sInstance)
                sInstance = new SEngineInstance;

            return sInstance;
        }

        void SEngineInstance::destroy(void)
        {
            if (sInstance)
            {
                sInstance->kill();
                delete sInstance;
            }

            sInstance = NULL;
        }

        void SEngineInstance::setMode(const MODE_NAME &mode)
        {
            mEngineMode = mode;
        }

        void SEngineInstance::setTargetServer(Kiaro::Common::C8 *address, Kiaro::Common::U16 port)
        {
            mTargetServerAddress = address;
            mTargetServerPort = port;
        }

        void SEngineInstance::setGame(const std::string &gameName)
        {
            mGameName = gameName;
        }

        bool SEngineInstance::isDedicated(void)
        {
            return mEngineMode & Kiaro::Engine::MODE_DEDICATED;
        }

        irr::IrrlichtDevice *SEngineInstance::getIrrlichtDevice(void)
        {
            return mIrrlichtDevice;
        }

        irr::scene::ISceneManager *SEngineInstance::getSceneManager(void)
        {
            return mSceneManager;
        }

        Kiaro::Common::S32 SEngineInstance::start(const Kiaro::Common::S32 &argc, Kiaro::Common::C8 *argv[])
        {
            mRunning = true;

            std::cout << "SEngineInstance: Running game '" << mGameName << "'" << std::endl;

            // Add the game search path
            if (PHYSFS_mount(mGameName.c_str(), NULL, 1) == 0)
            {
               mRunning = false;

               std::cerr << "SEngineInstance: Failed to mount game directory '" << mGameName << "'. Reason: ";
               std::cerr << PHYSFS_getLastError() << std::endl;
               return -1;
            }
            else
                std::cout << "SEngineInstance: Mounted game directory '" << mGameName << "' successfully." << std::endl;

            // TODO (Robert MacGregor#9): Return error codes for Lua, renderer, GUI and the netcode
            mRunning = this->initializeLua(argc, argv) == 0 ? true : false;
            if (!mRunning)
                return 1;

            this->initializeRenderer();
            this->initializeGUI();

            this->initializeSound();
            this->initializeNetwork();

            this->runGameLoop();

            return 1;
        }

        void SEngineInstance::kill(void)
        {
            if (!mRunning)
                return;

            //if (mServer)
            //{
            //    mServer->
           // }

            std::cout << "SEngineInstance: Killed via Kill()" << std::endl;

            mRunning = false;
        }

        SEngineInstance::SEngineInstance(void) : mEngineMode(Kiaro::Engine::MODE_CLIENT), mIrrlichtDevice(NULL), mTargetServerAddress("127.0.0.1"), mTargetServerPort(11595), mClient(NULL), mServer(NULL),
        mRunning(false), mClearColor(Kiaro::Common::ColorRGBA(0, 0, 0, 0)), mLuaState(NULL), mCurrentScene(NULL)
        {

        }

        SEngineInstance::~SEngineInstance(void)
        {
            std::cout << "SEngineInstance: Deinitializing ..." << std::endl;

            // TODO: Check the destroy order
            if (mClient)
            {
                mClient->disconnect();
                Kiaro::Network::SClient::destroy();

                mClient = NULL;
            }

            if (mServer)
            {
                Kiaro::Network::SServer::destroy();

                mServer = NULL;
            }

            Kiaro::Engine::SInputListener::destroy();
            Kiaro::Support::SScheduler::destroy();

            if (mIrrlichtDevice)
            {
                mIrrlichtDevice->drop();
                mIrrlichtDevice = NULL;
            }

            if (mLuaState)
            {
                lua_close(mLuaState);
                mLuaState = NULL;
            }

            PHYSFS_deinit();
            enet_deinitialize();
        }

        void SEngineInstance::networkUpdate(void)
        {
            // FIXME (Robert MacGregor#9): Pass in the time delta
            if (mClient)
                mClient->networkUpdate();

            if (mServer)
                mServer->networkUpdate(0.0f);
        }

        lua_State *SEngineInstance::getLuaState(void)
        {
            return mLuaState;
        }

        void SEngineInstance::initializeGUI(void)
        {
            // Start up CEGUI (if we're a client)
            if (mEngineMode == Kiaro::Engine::MODE_CLIENTCONNECT || mEngineMode == Kiaro::Engine::MODE_CLIENT)
            {
                // We don't need the OS cursor
                mIrrlichtDevice->getCursorControl()->setVisible(false);

                CEGUI::IrrlichtRenderer::bootstrapSystem(*mIrrlichtDevice);

                // Setup our resource directories
                // TODO (Robert MacGregor#9): Build a resource provider for CEGUI that implements PhysFS
                CEGUI::DefaultResourceProvider* resourceProvider = static_cast<CEGUI::DefaultResourceProvider*>( CEGUI::System::getSingleton().getResourceProvider());

                std::string basePath = "./";
                basePath += mGameName + "/";

                resourceProvider->setResourceGroupDirectory("fonts", basePath + "fonts/");
                resourceProvider->setResourceGroupDirectory("ui", basePath + "ui/");
                resourceProvider->setResourceGroupDirectory("global", basePath);

                CEGUI::SchemeManager::getSingleton().createFromFile("TaharezLook.scheme", "ui");
                CEGUI::FontManager::getSingleton().createFromFile( "DejaVuSans-10.font", "fonts" );

                // Set the defaults
                CEGUI::GUIContext &guiContext = CEGUI::System::getSingleton().getDefaultGUIContext();

                guiContext.setDefaultFont( "DejaVuSans-10" );
                guiContext.getMouseCursor().setDefaultImage( "TaharezLook/MouseArrow" );
                guiContext.getMouseCursor().setImage(guiContext.getMouseCursor().getDefaultImage());

                std::cout << "SEngineInstance: Initialized the GUI system." << std::endl;
            }
        }

        Kiaro::Common::U32 SEngineInstance::initializeLua(const Kiaro::Common::S32 &argc, Kiaro::Common::C8 *argv[])
        {
            // Initialize Lua
            // FIXME (Robert MacGregor#9): Init and call our Lua main method within the try, catch
            mLuaState = luaL_newstate();
            luaL_checkversion(mLuaState);
            lua_gc(mLuaState, LUA_GCSTOP, 0);
            luaL_openlibs(mLuaState);
            lua_gc(mLuaState, LUA_GCRESTART, 0);

            // Now create the callback tables
            lua_createtable(mLuaState, 0, 0);
            lua_setglobal(mLuaState, "GameServer");
            lua_createtable(mLuaState, 0, 0);
            lua_setglobal(mLuaState, "GameClient");

            // The game window is used for callbacks from the window
            lua_createtable(mLuaState, 0, 0);
            lua_setglobal(mLuaState, "GameWindow");

            // Load up the main file
            // TODO: Implement PhysFS in Lua
            const char *mainFileBase = PHYSFS_getRealDir("main.lua");
            if (!mainFileBase)
            {
                std::cerr << "SEngineInstance: Failed to locate main.lua file!" << std::endl;
                return 1;
            }

            std::string mainLuaFile = mainFileBase;
            mainLuaFile += "/main.lua";

            if (luaL_dofile(mLuaState, mainLuaFile.c_str()) >= 1)
            {
               std::cerr << "Failed to load main.lua!" << std::endl;
               return 2;
            }
            // Call the main(argv) method
            lua_getglobal(mLuaState, "main");

            // Create the parameter table
            lua_newtable(mLuaState);

            // Push the command line table
            lua_newtable(mLuaState);

            for (Kiaro::Common::S32 iteration = 0; iteration < argc; iteration++)
            {
                lua_pushinteger(mLuaState, iteration + 1);
                lua_pushstring(mLuaState, argv[iteration]);
                lua_settable(mLuaState, -3);
            }
            lua_call(mLuaState, 2, 0);

            std::cout << "SEngineInstance: Initialized Lua " << std::endl;

            return 0;
        }

        void SEngineInstance::initializeRenderer(void)
        {
            // Handle Execution Flag
            irr::video::E_DRIVER_TYPE videoDriver = irr::video::EDT_OPENGL;

            if (mEngineMode == Kiaro::Engine::MODE_DEDICATED)
                videoDriver = irr::video::EDT_NULL;

            // Init the Input listener
            Kiaro::Engine::SInputListener *inputListener = Kiaro::Engine::SInputListener::getPointer();

            // Start up Irrlicht
            mIrrlichtDevice = irr::createDevice(videoDriver, irr::core::dimension2d<Kiaro::Common::U32>(640, 480), 32, false, false, false, inputListener);
            mIrrlichtDevice->setWindowCaption(L"Kiaro Game Engine");

            // Grab the scene manager and store it to reduce a function call
            mSceneManager = mIrrlichtDevice->getSceneManager();

            // Initialize the main scene and set it
            // TODO (Robert MacGregor#9): Only initialize when running as a client.
            mMainScene = new Kiaro::Engine::CSceneGraph();
            this->setSceneGraph(mMainScene);

            std::cout << "SEngineInstance: Irrlicht version is " << mIrrlichtDevice->getVersion() << std::endl;
            std::cout << "SEngineInstance: Initialized renderer. " << std::endl;
        }

        Kiaro::Common::U32 SEngineInstance::initializeNetwork(void)
        {
            // Print the linked E-Net version
            const ENetVersion enetVersion = enet_linked_version();
            std::cout << "SEngineInstance: E-Net Version is " << ENET_VERSION_GET_MAJOR(enetVersion) << ".";
            std::cout << ENET_VERSION_GET_MINOR(enetVersion) << "." << ENET_VERSION_GET_PATCH(enetVersion) << std::endl;

            // Catch if the netcode somehow doesn't initialize correctly.
            if (enet_initialize() < 0)
            {
                std::cerr << "SEngineInstance: Failed to initialize the network!" << std::endl;
                return 1;
            }

            // Initialize the client or server ends
            switch (mEngineMode)
            {
                case Kiaro::Engine::MODE_CLIENT:
                {
                    mClient = Kiaro::Network::SClient::getPointer();
                    break;
                }

                case Kiaro::Engine::MODE_CLIENTCONNECT:
                {
                    mClient = Kiaro::Network::SClient::getPointer();
                    mClient->connect(mTargetServerAddress, mTargetServerPort, 5000);

                    if (!mClient->getIsConnected())
                    {
                        std::cerr << "SEngineInstance: Failed to connect to remote host with server flag" << std::endl;

                        Kiaro::Network::SClient::destroy();
                    }

                    break;
                }

                case Kiaro::Engine::MODE_DEDICATED:
                {
                    mServer = Kiaro::Network::SServer::getPointer("0.0.0.0", 11595, 32);
                    break;
                }
            }

            std::cout << "SEngineInstance: Initialized network." << std::endl;
            return 0;
        }

        void SEngineInstance::runGameLoop(void)
        {
            // What is our current display size?
            irr::core::dimension2d<irr::u32> lastDisplaySize = mIrrlichtDevice->getVideoDriver()->getScreenSize();

            // Start the Loop
            Kiaro::Common::F32 deltaTimeSeconds = 0.0f;

            // Setup and start the network time pulses
            EngineTimePulseDelegate *timePulse = new EngineTimePulseDelegate(new EngineTimePulseDelegate::MemberDelegateType<SEngineInstance>(this, &SEngineInstance::networkUpdate));
            Kiaro::Support::SScheduler::getPointer()->schedule(timePulse, ENGINE_TICKRATE, true);

            // The GUI and sound systems run independently of our network time pulse
            while (mRunning && mIrrlichtDevice->run())
            {
                try
                {
                    // Update all our subsystems
                    Kiaro::Support::FTime::timer timerID = Kiaro::Support::FTime::startTimer();

                    Kiaro::Support::SScheduler::getPointer()->update();

                    // Send a time pulse to all the client end functionality
                    if (mEngineMode == Kiaro::Engine::MODE_CLIENT || mEngineMode == Kiaro::Engine::MODE_CLIENTCONNECT)
                    {
                        irr::core::dimension2d<Kiaro::Common::U32> currentDisplaySize = mIrrlichtDevice->getVideoDriver()->getScreenSize();
                        if (lastDisplaySize != currentDisplaySize)
                        {
                            CEGUI::Sizef newDisplaySize(currentDisplaySize.Width, currentDisplaySize.Height);
                            CEGUI::System::getSingleton().notifyDisplaySizeChanged(newDisplaySize);

                            lastDisplaySize = currentDisplaySize;
                        }

                        CEGUI::System::getSingleton().injectTimePulse(deltaTimeSeconds);

                        // Since we're a client, render the frame right after updating
                        mIrrlichtDevice->getVideoDriver()->beginScene(true, true, mClearColor);
                        mSceneManager->drawAll();

                        CEGUI::System::getSingleton().renderAllGUIContexts();

                        mIrrlichtDevice->getVideoDriver()->endScene();
                    }

                    deltaTimeSeconds = Kiaro::Support::FTime::stopTimer(timerID);
                }
                catch(std::exception &e)
                {
                    std::cerr << "SEngineInstance: An internal exception of type '" << typeid(e).name() << "' has occurred: \"" << e.what() << "\"" << std::endl;

                    // Something is probably up, we should leave.
                    if (mClient)
                    {
                        mClient->disconnect();
                        mClient = NULL;

                        Kiaro::Network::SClient::destroy();
                    }

                    // Servers just drop off the client that it last processed
                    if (mServer)
                    {
                        Kiaro::Network::CClient *lastClient = mServer->getLastPacketSender();

                        // TODO (Robert MacGregor#9): Handle for if an exception is thrown AND we are running as a listen server.
                        if (lastClient)
                            lastClient->disconnect("Internal Exception");
                    }
                }
            }
        }

        Kiaro::Common::U32 SEngineInstance::initializeSound(void)
        {
            // Don't initialize the sound engine if we're running a dedicated server
            if (mEngineMode == Kiaro::Engine::MODE_DEDICATED)
                return 0;

            #ifndef ENGINE_BUILD_SOUNDENGINE
                std::cout << "SEngineInstance: Built without sound support. There will be no audio playback. " << std::endl;
            #else
            #endif // ENGINE_BUILD_SOUNDENGINE

            return 0;
        }

        void SEngineInstance::setSceneGraph(Kiaro::Engine::CSceneGraph *graph)
        {
            if (mCurrentScene)
                mCurrentScene->setVisible(false);

            mCurrentScene = graph;
            mCurrentScene->setVisible(true);
        }
    } // End Namespace Engine
} // End Namespace Kiaro