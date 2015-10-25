/************************************************************************************

Filename    :   ExperimentalWorld.cpp
Content     :   First-person view test application for Oculus Rift - Implementation
Created     :   October 4, 2012
Authors     :   Michael Antonov, Andrew Reisse, Steve LaValle, Dov Katz
                Peter Hoff, Dan Goodman, Bryan Croteau

Copyright   :   Copyright 2012 Oculus VR, LLC. All Rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*************************************************************************************/

#include "ExperimentalWorld.h"
#include "Kernel/OVR_Threads.h"
#include "Util/Util_SystemGUI.h"
#include <algorithm>

#if defined(OVR_OS_MS)
#include "OVR_CAPI_D3D.h"
#endif


OVR_DISABLE_MSVC_WARNING(4996) // "scanf may be unsafe"


Posef CockpitPanelPose[5];
Vector2f CockpitPanelSize[5];
Recti CockpitClipRect[5];

const bool ExperimentalWorldApp::AllowMsaaTargets[ExperimentalWorldApp::Rendertarget_LAST] =
{
    true, //Rendertarget_Left,
    true, //Rendertarget_Right,
    true, //Rendertarget_BothEyes,
    false, //Rendertarget_Hud,
    false, //Rendertarget_Menu
};

const bool ExperimentalWorldApp::UseDepth[ExperimentalWorldApp::Rendertarget_LAST] =
{
    true, //Rendertarget_Left,
    true, //Rendertarget_Right,
    true, //Rendertarget_BothEyes,
    false, //Rendertarget_Hud,
    false, //Rendertarget_Menu
};


//-------------------------------------------------------------------------------------
// ***** ExperimentalWorldApp

ExperimentalWorldApp::ExperimentalWorldApp() :
    pRender(0),
    RenderParams(),
    WindowSize(1280,800),
    HmdDisplayAcquired(false),
    FirstScreenInCycle(0),
    SupportsSrgbSwitching(true),        // May be proven false below.
    SupportsMultisampling(true),        // May be proven false below.
    SupportsDepthMultisampling(true),   // May be proven false below.
    ActiveGammaCurve(1.0f),
    SrgbGammaCurve(2.2f),
    MirrorIsSrgb(false),

    //RenderTargets()
    //MsaaRenderTargets()
    //DrawEyeTargets(),
    Hmd(0),
    HmdDesc(),
    //EyeRenderDesc[2];
    //Projection[2];          // Projection matrix for eye.
    //OrthoProjection[2];     // Projection for 2D.
    //EyeRenderPose[2];       // Poses we used for rendering.
    //EyeTexture[2];
    //EyeRenderSize[2];       // Saved render eye sizes; base for dynamic sizing.
    UsingDebugHmd(false),

    SecondsOfFpsMeasurement(1.0f),
    FrameCounter(0),
    TotalFrameCounter(0),
    SecondsPerFrame(0.f),
    FPS(0.f),
    LastFpsUpdate(0.0),
    LastUpdate(0.0),

    MainFilePath(),
    CollisionModels(),
    GroundCollisionModels(),

    LoadingState(LoadingState_Frame0),

    InteractiveMode(true),
    HaveVisionTracking(false),
    HavePositionTracker(false),
    HaveHMDConnected(false),
    HaveSync(true),

    Recording(false),
    Replaying(false),
    LastSyncTime(0.0),

    LastGamepadState(),

    ThePlayer(),
    MainScene(),
    LoadingScene(),
    SmallGreenCube(),
    SmallOculusCube(),
    SmallOculusGreenCube(),
    SmallOculusRedCube(),

    OculusCubesScene(),
    GreenCubesScene(),
    RedCubesScene(),

    TextureRedCube(),
    TextureGreenCube(),
    TextureOculusCube(),

    CockpitPanelTexture(),

    HmdFrameTiming(),
    HmdStatus(0),    

    NotificationTimeout(0.0),

    HmdSettingsChanged(false),

    RendertargetIsSharedByBothEyes(false),
    DynamicRezScalingEnabled(false),





    HasInputState(false),    
    InterAxialDistance(0.0f),
    MonoscopicRenderMode(Mono_Off),
    PositionTrackingScale(1.0f),
    ScaleAffectsEyeHeight(false),
    DesiredPixelDensity(1.0f),
    FovScaling(0.0f),
    NearClip(0.01f),
    FarClip(10000.0f),
    DepthModifier(NearLessThanFar),
    DepthFormatOption(DepthFormatOption_D32f),
    SceneRenderCountType(SceneRenderCount_FixedLow),
    SceneRenderCountLow(1),
    SceneRenderCountHigh(10),
    SceneRenderWasteCpuTimePreRender(0),
    SceneRenderWasteCpuTimeEachRender(0),
    SceneRenderWasteCpuTimePreSubmit(0),
    DrawFlushMode(DrawFlush_Off),
    DrawFlushCount(10),
    MenuHudMovementMode(MenuHudMove_FixedToFace),
    MenuHudMovementRadius(36.0f),
    MenuHudMovementDistance(0.35f),
    MenuHudMovementRotationSpeed(400.0f),
    MenuHudMovementTranslationSpeed(4.0f),
    MenuHudTextPixelHeight(22.0f),
    MenuHudDistance(0.8f),
    MenuHudSize(1.25f),     // default = 1.0f / MenuHudDistance
    MenuHudMaxPitchToOrientToHeadRoll(45.0f),
	MenuHudAlwaysOnMirrorWindow(false),

    TimewarpRenderIntervalEnabled(true),
    TimewarpRenderIntervalInSeconds(0.0f),
    FreezeEyeUpdate(false),
    FreezeEyeOneFrameRendered(false),
    ComputeShaderEnabled(false),

#ifdef OVR_OS_MS
    LayersEnabled(true),             // Use layers at all
#else
    LayersEnabled(false),
#endif
    Layer0HighQuality(false),
    Layer0Depth(false),
    Layer1Enabled(false),
    Layer1HighQuality(false),
    Layer2Enabled(false),
    Layer2HighQuality(false),
    Layer3Enabled(false),
    Layer3HighQuality(false),
    Layer4Enabled(false),
    Layer4HighQuality(false),
    Layer234Size(1.0f),
    LayerDebugEnabled(false),
    LayerCockpitEnabled(0),
    LayerCockpitHighQuality(false),
    LayerHudMenuEnabled(true),
    LayerHudMenuHighQuality(false),

    CenterPupilDepthMeters(0.05f),
    ForceZeroHeadMovement(false),
    MultisampleRequested(true),
    MultisampleEnabled(true),
    SrgbRequested(true),
    AnisotropicSample(true),
    TextureOriginAtBottomLeft(false),
#if defined(OVR_OS_LINUX)
    LinuxFullscreenOnDevice(false),
#endif
    IsLowPersistence(true),
    DynamicPrediction(true),
    PositionTrackingEnabled(true),
    MirrorToWindow(true),

    DistortionClearBlue(0),
    ShiftDown(false),
    CtrlDown(false),
    IsLogging(false),

    PerfHudMode(ovrPerfHud_Off),
    DebugHudStereoPresetMode(DebugHudStereoPreset_Free),
    DebugHudStereoMode(ovrDebugHudStereo_Off),
    DebugHudStereoGuideInfoEnable(true),
    DebugHudStereoGuidePosition(0.0f, 0.0f, -1.5f),
    DebugHudStereoGuideSize(1.0f, 1.0f),
    DebugHudStereoGuideYawPitchRollDeg(0.0f, 0.0f, 0.0f),
    DebugHudStereoGuideYawPitchRollRad(0.0f, 0.0f, 0.0f),
    DebugHudStereoGuideColor(1.0f, 0.5f, 0.1f, 0.8f),

    SceneMode(Scene_World),
    GridDisplayMode(GridDisplay_None),
    GridMode(Grid_Lens),
    TextScreen(Text_None),
    ComfortTurnMode(ComfortTurn_Off),
    BlocksShowType(0),
    BlocksShowMeshType(0),
    BlocksSpeed(1.0f),
    BlocksCenter(0.0f, 0.0f, 0.0f),
    BlocksSize(20.0f, 20.0f, 20.0f),
    BlocksHowMany(10),
    BlocksMovementRadius(1.0f),
    BlocksMovementType(0),
    BlocksMovementScale(0.5f),

    Menu(),
    ShortcutChangeMessageEnable(true),

    Profiler(),
    IsVisionLogging(false),
    SensorSampleTimestamp(0.0),
    MenuPose(),
    MenuIsRotating(false),
    MenuIsTranslating(false)
{
    OVR_ExceptionHandler.SetExceptionListener(&OVR_GUIExceptionListener, 0); // Use GUI popup?
    OVR_ExceptionHandler.SetPathsFromNames("Oculus", "ExperimentalWorld"); // File dump?
    OVR_ExceptionHandler.EnableReportPrivacy(true); // Dump less info?
    OVR_ExceptionHandler.Enable(true); // Enable handler

    EyeRenderSize[0] = EyeRenderSize[1] = Sizei(0);

    ViewFromWorld[0].SetIdentity();
    ViewFromWorld[1].SetIdentity();

    for ( int i = 0; i < Rendertarget_LAST; i++ )
    {
        DrawEyeTargets[i] = nullptr;
    }

    // EyeRenderDesc[], EyeTexture[] : Initialized in CalculateHmdValues()

    memset(LayerList, 0, sizeof(LayerList));

    // Set up the "cockpit"
    Vector3f CockpitCenter = Vector3f ( 7.75f, 1.6f, -1.75f );
    // Lower middle
    CockpitPanelPose[0] = Posef ( Quatf ( Axis_X, -0.5f ), Vector3f (  0.0f, -0.4f, 0.0f ) + CockpitCenter );
    CockpitPanelSize[0] = Vector2f ( 0.5f, 0.3f );
    CockpitClipRect [0] = Recti ( 0, 373, 387, 512-373 );
    // Left side
    CockpitPanelPose[1] = Posef ( Quatf ( Axis_Y,  0.7f ), Vector3f ( -0.6f, -0.4f, 0.2f ) + CockpitCenter );
    CockpitPanelSize[1] = Vector2f ( 0.5f, 0.3f );
    CockpitClipRect [1] = Recti ( 272, 0, 512-272, 103 );
    CockpitPanelPose[2] = Posef ( Quatf ( Axis_Y,  0.5f ), Vector3f ( -0.4f,  0.2f, 0.0f ) + CockpitCenter );
    CockpitPanelSize[2] = Vector2f ( 0.3f, 0.4f );
    CockpitClipRect [2] = Recti ( 0, 0, 132, 339 );
    // Right side
    CockpitPanelPose[3] = Posef ( Quatf ( Axis_Y, -0.7f ), Vector3f ( 0.6f, -0.4f, 0.2f ) + CockpitCenter );
    CockpitPanelSize[3] = Vector2f ( 0.5f, 0.3f );
    CockpitClipRect [3] = Recti ( 272, 0, 512-272, 103 );
    CockpitPanelPose[4] = Posef ( Quatf ( Axis_Y, -0.5f ), Vector3f ( 0.4f,  0.2f, 0.0f ) + CockpitCenter );
    CockpitPanelSize[4] = Vector2f ( 0.3f, 0.4f );
    CockpitClipRect [4] = Recti ( 132, 0, 258-132, 173 );


    HandStatus[0] = HandStatus[1] = 0;
}

ExperimentalWorldApp::~ExperimentalWorldApp()
{
    ClearScene();
    DestroyRendering();
    ovr_Shutdown();
}

void ExperimentalWorldApp::DestroyRendering()
{
    CleanupDrawTextFont();

    if (Hmd)
    {
        // Delete any render targets and the mirror window, which may be
        // using ovrSwapTextureSet or ovrTexture underneath and need to
        // call in the Hmd to be deleted (since we're about to destroy
        // the hmd).
        for (size_t i = 0; i < OVR_ARRAY_COUNT(RenderTargets); ++i)
        {
            RenderTargets[i].pColorTex.Clear();
            RenderTargets[i].pDepthTex.Clear();
            MsaaRenderTargets[i].pColorTex.Clear();
            MsaaRenderTargets[i].pDepthTex.Clear();
        }

        for (size_t i = 0; i < OVR_ARRAY_COUNT(EyeTexture); ++i)
        {
            EyeTexture[i].Clear();
            EyeDepthTexture[i].Clear();
        }

        MirrorTexture.Clear();

        // Need to explicitly clean these up because they can contain SwapTextureSets,
        // and we need to release those before we destroy the HMD connection.
        CockpitPanelTexture.Clear();
        TextureOculusCube.Clear();
        TextureGreenCube.Clear();
        TextureRedCube.Clear();

        pPlatform->DestroyGraphics();
        pRender = nullptr;

        ovr_Destroy(Hmd);
        Hmd = nullptr;
    }

    CollisionModels.ClearAndRelease();
    GroundCollisionModels.ClearAndRelease();

    // Setting this will make sure OnIdle() doesn't continue rendering until we reacquire hmd
    HmdDisplayAcquired = false;
}

// Example CAPI log callback
static void OVR_CDECL LogCallback(uintptr_t /*userData*/, int level, const char* message)
{
    LogMessageType logMessageType = OVR::Log_Text;

    if (level == ovrLogLevel_Debug)
        logMessageType = OVR::Log_Debug;
    else if (level == ovrLogLevel_Error)
        logMessageType = OVR::Log_Error;

    Log::GetGlobalLog()->LogMessage(logMessageType, "%s", message);
}

int ExperimentalWorldApp::OnStartup(int argc, const char** argv)
{
    Argc = argc;
    Argv = argv;

    OVR::Thread::SetCurrentThreadName("OWDMain");

    // *** Oculus HMD Initialization

    // Example use of ovr_Initialize() to specify a log callback.
    // The log callback can be called from other threads until ovr_Shutdown() completes.
    ovrInitParams params = {0, 0, nullptr, 0, 0, OVR_ON64("")};


    OVR::String replayFile;

    for (int i = 1; i < argc; i++)
    {
        const char* argStr = argv[i];

        while (*argStr == '-' || *argStr == '/')
        {
            ++argStr;
        }


        if (!OVR_stricmp(argStr, "automation"))
        {
            InteractiveMode = false;
        }

        if (!OVR_stricmp(argStr, "replay"))
        {
            if ( i <= argc - 1 ) // next arg is the filename
            {
               replayFile = argv[ i + 1 ]; 
            }
        }
    }


    params.LogCallback = LogCallback;
    ovrResult error = ovr_Initialize(&params);

    ovr_TraceMessage(ovrLogLevel_Info, "Oculus World Demo OnStartup");

    if (error != ovrSuccess)
    {
        DisplayLastErrorMessageBox("ovr_Initialize failure.");
        return 1;
    }

    if ( !replayFile.IsEmpty() )
    {
        ovr_SetString( nullptr, "server:replayInit", replayFile.ToCStr() );
    }

    return InitializeRendering(true);
}

int ExperimentalWorldApp::InitializeRendering(bool firstTime)
{
    ovrGraphicsLuid luid;
	ovrResult error = ovr_Create(&Hmd, &luid);

    if (error != ovrSuccess)
    {
        if (firstTime)
        {
            DisplayLastErrorMessageBox("Unable to create HMD.");
            return 1;
        }
        else
        {
            HmdDisplayAcquired = false;
            return 0;
        }
    }

    HmdDisplayAcquired = true;

    // Did we end up with a debug HMD?
    UsingDebugHmd = (ovr_GetEnabledCaps(Hmd) & ovrHmdCap_DebugDevice) != 0;

    HmdDesc = ovr_GetHmdDesc(Hmd);

    // In Direct App-rendered mode, we can use smaller window size,
    // as it can have its own contents and isn't tied to the buffer.
    WindowSize = Sizei(1100, 618);//Sizei(960, 540); avoid rotated output bug.


    // ***** Setup System Window & rendering.

	if (!SetupWindowAndRendering(firstTime, Argc, Argv, luid))
    {
        if (firstTime)
        {
            DisplayLastErrorMessageBox("Unable to initialize rendering");
            return 1;
        }
        else
        {
            HmdDisplayAcquired = false;
            GetPlatformCore()->SetNotificationOverlay(0, 28, 8, "Unable to initialize rendering");
            return 0;
        }
    }

    NotificationTimeout = ovr_GetTimeInSeconds() + 10.0f;

    PositionTrackingEnabled = (HmdDesc.DefaultTrackingCaps & ovrTrackingCap_Position) ? true : false;

    // *** Configure HMD Stereo settings.

    error = CalculateHmdValues();
    if (error != ovrSuccess)
    {
        if (firstTime)
        {
            DisplayLastErrorMessageBox("Unable to initialize rendering");
            return 1;
        }
        else
        {
            GetPlatformCore()->SetNotificationOverlay(0, 28, 8, "Unable to initialize rendering");

            // Destroy what was initialized
            HandleOvrError(error);
            return 0;
        }
    }

    // Query eye height.
    ThePlayer.UserEyeHeight = ovr_GetFloat(Hmd, OVR_KEY_EYE_HEIGHT, ThePlayer.UserEyeHeight);
    ThePlayer.BodyPos.y     = ThePlayer.UserEyeHeight;
    // Center pupil for customization; real game shouldn't need to adjust this.
    CenterPupilDepthMeters  = ovr_GetFloat(Hmd, "CenterPupilDepth", 0.0f);


    ThePlayer.bMotionRelativeToBody = false;  // Default to head-steering for DK1

    if (UsingDebugHmd)
        Menu.SetPopupMessage("NO HMD DETECTED");
    else if (!(ovr_GetTrackingState(Hmd, 0.0f, ovrTrue).StatusFlags & ovrStatus_HmdConnected))
        Menu.SetPopupMessage("NO SENSOR DETECTED");
    else
        Menu.SetPopupMessage("Please put on Rift");

    // Give first message 10 sec timeout, add border lines.
    Menu.SetPopupTimeout(10.0f, true);

    PopulateOptionMenu();

    // *** Identify Scene File & Prepare for Loading

    InitMainFilePath();
    PopulatePreloadScene();

    SetCommandLineMenuOption(Argc, Argv);

    LastUpdate = ovr_GetTimeInSeconds();

    // Create a layer list. This is slightly gratuitous - this app had only one thread,
    // so it really only needs the default list. So we're creating and using one just to
    // demonstrate/test the API.
    //OVR_ASSERT ( pCockpitLayerList == nullptr );
    //pCockpitLayerList = ovr_CreateLayerList ( Hmd );


    return 0;
}

// This function sets parameters based on command line arguments
// The argument should have the format --m:FeatureName=Value
// Feature supported:
//   scenecontent.drawrepeat.lowcount
//   scenecontent.renderscene
void ExperimentalWorldApp::SetCommandLineMenuOption(int argc, const char** argv)
{
    for (int iTok = 0; iTok < argc; ++iTok)
    {
        // Looking for the --m argument tag
        std::string token(argv[iTok]);
        if (token.compare("--m:"))
        {
            size_t start = token.find_first_of(":") + 1;
            size_t seperator = token.find_last_of("=");

            if (start != std::string::npos && seperator != std::string::npos)
            {
                OptionMenuItem *menuItem = nullptr;

                std::string label = token.substr(start, seperator - start);
                std::string value = token.substr(seperator + 1);

                // Lower case to make the string comparison case independent
                std::transform(label.begin(), label.end(), label.begin(), ::tolower);

                // Looking for the menu item to set
                if (label.compare("scenecontent.drawrepeat.lowcount") == 0)
                {
                    menuItem = Menu.FindMenuItem("Scene Content.Draw Repeat.Low count");
                }
                else if (label.compare("scenecontent.renderscene") == 0)
                {
                    menuItem = Menu.FindMenuItem("Scene Content.Rendered Scene ';");
                }

                // Setting the value of the feature
                if (menuItem != nullptr)
                {
                    menuItem->SetValue(OVR::String(value.c_str()));
                }
                else
                {
                    OVR_DEBUG_LOG(("[ExperimentalWorldApp::SetCommandLineMenuOption] Warning: Didn't find label for -> %s\r\n", label.c_str()));
                }
            }
        }
    }
}

bool ExperimentalWorldApp::SetupWindowAndRendering(bool firstTime, int argc, const char** argv, ovrGraphicsLuid luid)
{
    // *** Window creation

    // Don't recreate the window if we already created it
    if (firstTime)
    {
        void* windowHandle = pPlatform->SetupWindow(WindowSize.w, WindowSize.h);

        if (!windowHandle)
            return false;
    }

    // Report relative mouse motion in OnMouseMove
    pPlatform->SetMouseMode(Mouse_Relative);

    // *** Initialize Rendering

#if defined(OVR_OS_MS)
    const char* graphics = "d3d11";  //Default to DX11. Can be overridden below.
#else
    const char* graphics = "GL";
#endif

    // Select renderer based on command line arguments.
    // Example usage: App.exe -r d3d9 -fs
    // Example usage: App.exe -r GL
    // Example usage: App.exe -r GL -GLVersion 4.1 -GLCoreProfile -DebugEnabled
    for(int i = 1; i < argc; i++)
    {
        const bool lastArg = (i == (argc - 1)); // False if there are any more arguments after this.

        if(!OVR_stricmp(argv[i], "-r") && !lastArg)  // Example: -r GL
        {
            graphics = argv[++i];
        }
        else if(!OVR_stricmp(argv[i], "-MultisampleDisabled")) // Example: -MultisampleDisabled
        {
            MultisampleRequested = false;
        }
        else if(!OVR_stricmp(argv[i], "-DebugEnabled")) // Example: -DebugEnabled
        {
            RenderParams.DebugEnabled = true;
        }
        else if(!OVR_stricmp(argv[i], "-GLVersion") && !lastArg) // Example: -GLVersion 3.2
        {
            const char* version = argv[++i];
            sscanf(version, "%d.%d", &RenderParams.GLMajorVersion, &RenderParams.GLMinorVersion);
        }
        else if(!OVR_stricmp(argv[i], "-GLCoreProfile")) // Example: -GLCoreProfile
        {
            RenderParams.GLCoreProfile = true;
        }
        else if(!OVR_stricmp(argv[i], "-GLCoreCompatibilityProfile")) // Example: -GLCompatibilityProfile
        {
            RenderParams.GLCompatibilityProfile = true;
        }
        else if(!OVR_stricmp(argv[i], "-GLForwardCompatibleProfile")) // Example: -GLForwardCompatibleProfile
        {
            RenderParams.GLForwardCompatibleProfile = true;
        }
    }

    // Setup RenderParams.RenderAPIType
    if (OVR_stricmp(graphics, "GL") == 0)
    {
        RenderParams.RenderAPIType = ovrRenderAPI_OpenGL;
        TextureOriginAtBottomLeft = true;
    }
#if defined(OVR_OS_MS)
    else if (OVR_stricmp(graphics, "d3d11") == 0)
    {
        RenderParams.RenderAPIType = ovrRenderAPI_D3D11;
    }
#endif

    StringBuffer title;
    title.AppendFormat("Oculus World Demo %s : %s", graphics, HmdDesc.ProductName[0] ? HmdDesc.ProductName : "<unknown device>");
    pPlatform->SetWindowTitle(title);

    if (RenderParams.RenderAPIType == ovrRenderAPI_OpenGL)
    {
        // Right now, ovr_CreateSwapTextureSetGL cannot create MSAA textures.
        SupportsDepthMultisampling = false;

        // OpenGL only supports sRGB-correct rendering
        SupportsSrgbSwitching = false;
        SrgbRequested = true;
    }

    // Ideally we would use the created OpenGL context to determine multisamping support,
    // but that creates something of a chicken-and-egg problem which is easier to solve in
    // practice by the code below, as it's the only case where multisamping isn't supported
    // in modern computers of interest to us.
    #if defined(OVR_OS_MAC)
        if (RenderParams.GLMajorVersion < 3)
            SupportsMultisampling = false;
    #endif

    // Make sure the back buffer format is also sRGB correct, otherwise the blit of the mirror into the back buffer will be wrong
    RenderParams.SrgbBackBuffer = SrgbRequested;
    RenderParams.Resolution  = WindowSize;
#if defined(OVR_BUILD_DEBUG)
    RenderParams.DebugEnabled = true;
#endif

    pRender = pPlatform->SetupGraphics(Hmd, OVR_DEFAULT_RENDER_DEVICE_SET,
                                       graphics, RenderParams, luid); // To do: Remove the graphics argument to SetupGraphics, as RenderParams already has this info.
    return (pRender != nullptr);
}

// Custom formatter for Timewarp interval message.
static String FormatTimewarp(OptionVar* var)
{
    char    buff[64];
    float   timewarpInterval = *var->AsFloat();
    OVR_sprintf(buff, sizeof(buff), "%.1fms, %.1ffps",
                timewarpInterval * 1000.0f,
                ( timewarpInterval > 0.000001f ) ? 1.0f / timewarpInterval : 10000.0f);
    return String(buff);
}

static ovrFovPort g_EyeFov[2];  // store the current FOV globally for access by static display function

static String FormatMaxFromSideTan(OptionVar* var)
{
    OVR_UNUSED(var);
    char   buff[64];
    float horiz = (atan(g_EyeFov[0].LeftTan) + atan(g_EyeFov[1].RightTan)) * (180.0f / MATH_FLOAT_PI);
    float vert = (atan(g_EyeFov[0].UpTan) + atan(g_EyeFov[1].DownTan)) * (180.0f / MATH_FLOAT_PI);
    OVR_sprintf(buff, sizeof(buff), "%.1f x %.1f deg", horiz, vert);
    return String(buff);
}



void ExperimentalWorldApp::PopulateOptionMenu()
{
    // For shortened function member access.
    typedef ExperimentalWorldApp OWD;

    // *** Scene Content Sub-Menu

    // Test
    /*
        Menu.AddEnum("Scene Content.EyePoseMode", &FT_EyePoseState).AddShortcutKey(Key_Y).
        AddEnumValue("Separate Pose",  0).
        AddEnumValue("Same Pose",      1).
        AddEnumValue("Same Pose+TW",   2);
    */

    Menu.AddEnum("Scene Content.Rendered Scene ';'", &SceneMode).AddShortcutKey(Key_Semicolon).
                 AddEnumValue("World",        Scene_World).
                 AddEnumValue("Cubes",        Scene_Cubes).
                 AddEnumValue("Oculus Cubes", Scene_OculusCubes);

    // Toggle grid
    Menu.AddEnum("Scene Content.Grid Display 'G'",  &GridDisplayMode).AddShortcutKey(Key_G).
                 AddEnumValue("No Grid",                    GridDisplay_None).
                 AddEnumValue("Grid Only",                  GridDisplay_GridOnly).
                 AddEnumValue("Grid Direct (not stereo!)",  GridDisplay_GridDirect).
                 AddEnumValue("Grid And Scene",             GridDisplay_GridAndScene).
                            SetNotify(this, &OWD::HmdSettingChange); // GridDisplay_GridDirect needs to disable MSAA.
    Menu.AddEnum("Scene Content.Grid Mode 'H'",     &GridMode).AddShortcutKey(Key_H).
                 AddEnumValue("4-pixel RT-centered", Grid_Rendertarget4).
                 AddEnumValue("16-pixel RT-centered",Grid_Rendertarget16).
                 AddEnumValue("Lens-centered grid",  Grid_Lens);

    Menu.AddBool("Scene Content.Anisotropic Sampling", &AnisotropicSample).SetNotify(this, &OWD::SrgbRequestChange); // same sRGB function works fine

    // Animating blocks
    Menu.AddEnum("Scene Content.Animated Blocks.Type", &BlocksShowType).
                 AddShortcutKey(Key_B).SetNotify(this, &OWD::BlockShowChange).
                 AddEnumValue("None",  0).
                 AddEnumValue("Horizontal Circle", 1).
                 AddEnumValue("Vertical Circle",   2).
                 AddEnumValue("Bouncing Blocks",   3);
    Menu.AddEnum("Scene Content.Animated Blocks.Mesh", &BlocksShowMeshType).
                 AddEnumValue("Green",       0).
                 AddEnumValue("Oculus",      1).
                 AddEnumValue("Oculus Green",2).
                 AddEnumValue("Oculus Red",  3);
    Menu.AddInt  ("Scene Content.Animated Blocks.How Many Blocks", &BlocksHowMany, 1, 50, 1);
    Menu.AddFloat("Scene Content.Animated Blocks.Speed", &BlocksSpeed, 0.0f, 10.0f, 0.1f, "%.1f");
    Menu.AddEnum("Scene Content.Animated Blocks.Mesh", &BlocksMovementType).
                 AddEnumValue("Circle",             0).
                 AddEnumValue("Circle+Sine",        1).
                 AddEnumValue("Circle+Triangle",    2);
    Menu.AddFloat("Scene Content.Animated Blocks.Movement Radius", &BlocksMovementRadius, 0.1f, 100.0f, 0.1f, "%.2f");
    Menu.AddFloat("Scene Content.Animated Blocks.Movement Scale",  &BlocksMovementScale, 0.1f, 10.0f, 0.05f, "%.2f");
    Menu.AddFloat("Scene Content.Animated Blocks.Size X", &BlocksSize.x, 0.0f, 100.0f, 1.0f, "%.0f");
    Menu.AddFloat("Scene Content.Animated Blocks.Size Y", &BlocksSize.y, 0.0f, 100.0f, 1.0f, "%.0f");
    Menu.AddFloat("Scene Content.Animated Blocks.Size Z", &BlocksSize.z, 0.0f, 100.0f, 1.0f, "%.0f");
    

    Menu.AddInt ("Scene Content.Draw Repeat.Low count",  &SceneRenderCountLow, 1, 1000000, 1 );
    Menu.AddInt ("Scene Content.Draw Repeat.High count", &SceneRenderCountHigh, 1, 1000000, 1 );
    Menu.AddEnum("Scene Content.Draw Repeat.Load type",  &SceneRenderCountType).
                AddEnumValue("Fixed low",       SceneRenderCount_FixedLow).
                AddEnumValue("Sine wave 10s",   SceneRenderCount_SineTenSec).
                AddEnumValue("Square wave 10s", SceneRenderCount_SquareTenSec).
                AddEnumValue("Spikes",          SceneRenderCount_Spikes);

    Menu.AddEnum("Scene Content.Draw Flush.Flush Mode", &DrawFlushMode).
        AddEnumValue("Off", DrawFlush_Off).
        AddEnumValue("After Each Eye Render", DrawFlush_AfterEachEyeRender).
        AddEnumValue("After Eye Pair Render", DrawFlush_AfterEyePairRender);
    Menu.AddInt("Scene Content.Draw Flush.Max Flushes Per-Frame", &DrawFlushCount, 0, 100, 1);

    Menu.AddFloat("Scene Content.Waste CPU Time.Before Scene Draw (ms)", &SceneRenderWasteCpuTimePreRender, 0.0f, 20.0f, 0.1f);
    Menu.AddFloat("Scene Content.Waste CPU Time.After Each Repeated Draw (ms)", &SceneRenderWasteCpuTimeEachRender, 0.0f, 20.0f, 0.1f);
    Menu.AddFloat("Scene Content.Waste CPU Time.Before Frame Submit (ms)", &SceneRenderWasteCpuTimePreSubmit, 0.0f, 20.0f, 0.1f);

    // Render target menu
    Menu.AddBool( "Render Target.Share RenderTarget",  &RendertargetIsSharedByBothEyes).
                                                        AddShortcutKey(Key_F8).SetNotify(this, &OWD::HmdSettingChange);
    Menu.AddBool( "Render Target.Dynamic Res Scaling", &DynamicRezScalingEnabled).
                                                        AddShortcutKey(Key_F8, ShortcutKey::Shift_RequireOn);
    Menu.AddEnum( "Render Target.Monoscopic Render 'F7'",       &MonoscopicRenderMode).
                 AddEnumValue("Off",                            Mono_Off).
                 AddEnumValue("Zero IPD - !!nausea caution!!",  Mono_ZeroIpd).
                 AddEnumValue("Zero player scale",              Mono_ZeroPlayerScale).
                                                        AddShortcutKey(Key_F7).
                                                        SetNotify(this, &OWD::HmdSettingChangeFreeRTs);
    Menu.AddFloat("Render Target.Limit Fov",           &FovScaling, -1.0f, 1.0f, 0.02f,
                                                        "%.1f Degrees", 1.0f, &FormatMaxFromSideTan).
                                                        SetNotify(this, &OWD::HmdSettingChange);
    Menu.AddFloat("Render Target.Pixel Density",       &DesiredPixelDensity, 0.1f, 2.5, 0.025f, "%3.2f", 1.0f).
                                                        SetNotify(this, &OWD::HmdSettingChange);
    if (SupportsMultisampling)
    {
        Menu.AddBool("Render Target.MultiSample 'F4'",        &MultisampleRequested).AddShortcutKey(Key_F4).SetNotify(this, &OWD::RendertargetFormatChange);
    }
    if (SupportsSrgbSwitching)
    {
        Menu.AddBool("Render Target.sRGB Eye Buffers 'F5'",   &SrgbRequested)       .AddShortcutKey(Key_F5).SetNotify(this, &OWD::SrgbRequestChange);
    }

    // Currently only supported for D3D11 - TODO: Enable when switching depth formats is supported on GL as well
    if (RenderParams.RenderAPIType == ovrRenderAPI_D3D11)
    {
        Menu.AddEnum("Render Target.Depth.Format", &DepthFormatOption).
                                                        AddEnumValue("Depth32f",            DepthFormatOption_D32f).
                                                        AddEnumValue("Depth24_Stencil8",    DepthFormatOption_D24_S8).
                                                        AddEnumValue("Depth16",             DepthFormatOption_D16).
                                                        AddEnumValue("Depth32f_Stencil8",   DepthFormatOption_D32f_S8).
                                                        SetNotify(this, &OWD::HmdSettingChange);
    }


    Menu.AddFloat("Render Target.Depth.Near Clipping", &NearClip,
                                                        0.0001f, 10.00f, 0.0001f, "%.4f").
                                                        SetNotify(this, &OWD::HmdSettingChange);

    Menu.AddFloat("Render Target.Depth.Far Clipping", &FarClip,
                                                        1.0f, 100000.00f, 10.0f, "%.0f").
                                                        SetNotify(this, &OWD::HmdSettingChange);

    Menu.AddEnum( "Render Target.Depth.Depth Modifier", &DepthModifier).
                                                        AddEnumValue("Near < Far", NearLessThanFar).
                                                        AddEnumValue("Far < Near", FarLessThanNear).
                                                        AddEnumValue("Far < Near & Far Clip at infinity", FarLessThanNearAndInfiniteFarClip).
                                                        SetNotify(this, &OWD::HmdSettingChange);

    Menu.AddBool( "Timewarp.Freeze Eye Update 'C'",         &FreezeEyeUpdate).AddShortcutKey(Key_C);
    Menu.AddBool( "Timewarp.Render Interval Enabled 'I'",   &TimewarpRenderIntervalEnabled).AddShortcutKey(Key_I);
    Menu.AddFloat("Timewarp.Render Interval",               &TimewarpRenderIntervalInSeconds,
                                                             0.0001f, 1.00f, 0.0001f, "%.1f", 1.0f, &FormatTimewarp).
                                                             AddShortcutUpKey(Key_J).AddShortcutDownKey(Key_U);

    // Layers menu
    Menu.AddBool( "Layers.Layers Enabled 'Shift+L'",      &LayersEnabled).AddShortcutKey(Key_L, ShortcutKey::Shift_RequireOn).SetNotify(this, &OWD::HmdSettingChange);
    // Layer 0 is the main scene - can't turn it off!
    Menu.AddBool( "Layers.Main Layer HQ",           &Layer0HighQuality);
    Menu.AddBool( "Layers.World Layer Enabled",     &Layer1Enabled);
    Menu.AddBool( "Layers.World Layer HQ",          &Layer1HighQuality);
    Menu.AddBool( "Layers.Torso Layer Enabled",     &Layer2Enabled);
    Menu.AddBool( "Layers.Torso Layer HQ",          &Layer2HighQuality);
    Menu.AddBool( "Layers.Face Layer Enabled",      &Layer3Enabled);
    Menu.AddBool( "Layers.Face Layer HQ",           &Layer3HighQuality);
    Menu.AddBool( "Layers.Face2 Layer Enabled",     &Layer4Enabled);
    Menu.AddBool( "Layers.Face2 Layer HQ",          &Layer4HighQuality);
    Menu.AddFloat("Layers.Torso+Face Layer Size",   &Layer234Size, 0.0f, 10.0f, 0.05f);
    Menu.AddBool( "Layers.Debug Layer Enabled",     &LayerDebugEnabled);
    Menu.AddInt ( "Layers.Cockpit Enable Bitfield", &LayerCockpitEnabled, 0, 31, 1);
    Menu.AddBool( "Layers.Cockpit HighQuality",     &LayerCockpitHighQuality);
    Menu.AddBool( "Layers.HUD/Menu HighQuality",    &LayerHudMenuHighQuality);

    // Player menu
    Menu.AddFloat("Player.Position Tracking Scale", &PositionTrackingScale, 0.00f, 50.0f, 0.05f).
                                                    SetNotify(this, &OWD::EyeHeightChange);
    Menu.AddBool("Player.Scale Affects Player Height", &ScaleAffectsEyeHeight).SetNotify(this, &OWD::EyeHeightChange);
    Menu.AddFloat("Player.User Eye Height",         &ThePlayer.UserEyeHeight, 0.2f, 2.5f, 0.02f,
                                        "%4.2f m").SetNotify(this, &OWD::EyeHeightChange).
                                        AddShortcutUpKey(Key_Equal).AddShortcutDownKey(Key_Minus);
    Menu.AddFloat("Player.Center Pupil Depth",      &CenterPupilDepthMeters, 0.0f, 0.2f, 0.001f,
                                        "%4.3f m").SetNotify(this, &OWD::CenterPupilDepthChange);

    Menu.AddBool("Player.Body Relative Motion",     &ThePlayer.bMotionRelativeToBody).AddShortcutKey(Key_E);
    Menu.AddBool("Player.Zero Head Movement",       &ForceZeroHeadMovement) .AddShortcutKey(Key_F7, ShortcutKey::Shift_RequireOn);
    Menu.AddEnum("Player.Comfort Turn",             &ComfortTurnMode).
                 AddEnumValue("Off",                 ComfortTurn_Off).
                 AddEnumValue("30 degrees",          ComfortTurn_30Degrees).
                 AddEnumValue("45 degrees",          ComfortTurn_45Degrees);


    // Tracking menu
    Menu.AddTrigger("Tracking.Recenter HMD Pose 'R'").AddShortcutKey(Key_R).SetNotify(this, &OWD::ResetHmdPose);

    // Display menu
    Menu.AddBool("Display.Mirror Window.Enabled",    &MirrorToWindow).AddShortcutKey(Key_M).SetNotify(this, &OWD::MirrorSettingChange);
    Menu.AddInt("Display.Mirror Window.Width",       &WindowSize.w, 100, 4000).SetNotify(this, &OWD::WindowSizeChange);
    Menu.AddInt("Display.Mirror Window.Height",      &WindowSize.h, 100, 4000).SetNotify(this, &OWD::WindowSizeChange);
    Menu.AddTrigger("Display.Mirror Window.Set To Native HMD Res").SetNotify(this, &OWD::WindowSizeToNativeResChange);

    Menu.AddEnum( "Display.Border Clear Color",      &DistortionClearBlue).
                                                      SetNotify(this, &OWD::DistortionClearColorChange).
                                                      AddEnumValue("Black",  0).
                                                      AddEnumValue("Blue", 1);

#if defined(OVR_OS_WIN32) || defined(OVR_OS_WIN64)
#endif



    // Menu menu.
    Menu.AddBool( "Menu.Menu Change Messages",       &ShortcutChangeMessageEnable);
    Menu.AddBool( "Menu.Menu Visible 'Shift+Tab'",   &LayerHudMenuEnabled).AddShortcutKey(Key_Tab, ShortcutKey::Shift_RequireOn);
	Menu.AddBool( "Menu.Menu Always on Mirror Window", &MenuHudAlwaysOnMirrorWindow);
	
    Menu.AddFloat("Menu.HUD/Menu Text Pixel Height", &MenuHudTextPixelHeight, 3.0f, 75.0f, 0.5f);
    Menu.AddFloat("Menu.HUD/Menu Distance (meters)", &MenuHudDistance, 0.1f, 10.0f, 0.05f);
    Menu.AddFloat("Menu.HUD/Menu Scale",             &MenuHudSize, 0.1f, 10.0f, 0.1f);
    Menu.AddEnum( "Menu.Movement Mode",              &MenuHudMovementMode).
                AddEnumValue("Fixed to face",         MenuHudMove_FixedToFace).
                AddEnumValue("Drag at edge",          MenuHudMove_DragAtEdge).
                AddEnumValue("Recenter at edge",      MenuHudMove_RecenterAtEdge);
    Menu.AddFloat("Menu.Movement Radius (degrees)",  &MenuHudMovementRadius, 1.0f, 50.0f, 1.0f );
    Menu.AddFloat("Menu.Movement Position",          &MenuHudMovementDistance, 0.0f, 10.0f, 0.01f);
    Menu.AddFloat("Menu.Movement Rotation Speed (deg/sec)", &MenuHudMovementRotationSpeed, 10.0f, 2000.0f, 10.0f);
    Menu.AddFloat("Menu.Movement Translation Speed (m/sec)", &MenuHudMovementTranslationSpeed, 0.01f, 10.0f, 0.01f);
    Menu.AddFloat("Menu.Movement Pitch Zone For Roll (deg)", &MenuHudMaxPitchToOrientToHeadRoll, 0.0f, 80.0f);


    // Add DK2 options to menus only for that headset.
    if (HmdDesc.AvailableTrackingCaps & ovrTrackingCap_Position)
    {
        Menu.AddBool("Tracking.Positional Tracking 'X'",            &PositionTrackingEnabled).
                                                                     AddShortcutKey(Key_X).SetNotify(this, &OWD::HmdSettingChange);
    }

    Menu.AddEnum("SDK HUD.Perf HUD Mode 'F11'", &PerfHudMode).  AddEnumValue("Off", ovrPerfHud_Off).
                                                                AddEnumValue("Latency Timing", ovrPerfHud_LatencyTiming).
                                                                AddEnumValue("Render Timing", ovrPerfHud_RenderTiming).
                                                                AddEnumValue("Performance Headroom", ovrPerfHud_PerfHeadroom).
                                                                AddEnumValue("Version Info", ovrPerfHud_VersionInfo).
                                                                AddShortcutKey(Key_F11).
                                                                SetNotify(this, &OWD::PerfHudSettingChange);

    Menu.AddEnum("SDK HUD.Stereo Guide Mode", &DebugHudStereoMode). AddEnumValue("Off", ovrDebugHudStereo_Off).
                                                                    AddEnumValue("Quad", ovrDebugHudStereo_Quad).
                                                                    AddEnumValue("Quad with crosshair", ovrDebugHudStereo_QuadWithCrosshair).
                                                                    AddEnumValue("Crosshair at Infinity", ovrDebugHudStereo_CrosshairAtInfinity).
                                                                    SetNotify(this, &OWD::DebugHudSettingModeChange);

    Menu.AddEnum("SDK HUD.Stereo Settings.Stereo Guide Presets", &DebugHudStereoPresetMode).
                                                                    AddEnumValue("No preset", DebugHudStereoPreset_Free).
                                                                    AddEnumValue("Stereo guide off", DebugHudStereoPreset_Off).
                                                                    AddEnumValue("One meter square two meters away", DebugHudStereoPreset_OneMeterTwoMetersAway).
                                                                    AddEnumValue("Huge and bright", DebugHudStereoPreset_HugeAndBright).
                                                                    AddEnumValue("Huge and dim", DebugHudStereoPreset_HugeAndDim).
                                                                    SetNotify(this, &OWD::DebugHudSettingQuadPropChange);

        

    Menu.AddBool ("SDK HUD.Stereo Settings.Show Info", &DebugHudStereoGuideInfoEnable).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);

    Menu.AddFloat("SDK HUD.Stereo Settings.Width (m)", &DebugHudStereoGuideSize.x, 0.0f, 100.0f, 0.05f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Height (m)", &DebugHudStereoGuideSize.y, 0.0f, 100.0f, 0.05f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);

    Menu.AddFloat("SDK HUD.Stereo Settings.Position X (m)", &DebugHudStereoGuidePosition.x, -100.0f, 100.0f, 0.05f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Position Y (m)", &DebugHudStereoGuidePosition.y, -100.0f, 100.0f, 0.05f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Position Z (m)", &DebugHudStereoGuidePosition.z, -100.0f, 100.0f, 0.05f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);

    Menu.AddFloat("SDK HUD.Stereo Settings.Yaw (deg)",  &DebugHudStereoGuideYawPitchRollDeg.x, -180.0f, 180.0f, 1.0f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Pitch (deg)",&DebugHudStereoGuideYawPitchRollDeg.y, -180.0f, 180.0f, 1.0f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Roll (deg)", &DebugHudStereoGuideYawPitchRollDeg.z, -180.0f, 180.0f, 1.0f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);

    Menu.AddFloat("SDK HUD.Stereo Settings.Color R", &DebugHudStereoGuideColor.x, 0.0f, 1.0f, 0.01f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Color G", &DebugHudStereoGuideColor.y, 0.0f, 1.0f, 0.01f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Color B", &DebugHudStereoGuideColor.z, 0.0f, 1.0f, 0.01f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Color A", &DebugHudStereoGuideColor.w, 0.0f, 1.0f, 0.01f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);

}


// Return value is true on success.
// menuFullName = the path without the keyboard shortcut (e.g. "Perf HUD.Mode 'F11'" -> "Perf HUD.Mode")
// All comparisons are case insensitive.
// Enum types will look for the enum that matches newValue.
// Bool types will regard "", "0" and "false" as false, anything else as true.
// Trigger types ignore newValue. All menu items will trigger on setting.
// So for example these all work:
// SetMenuValue ( "perf hud.mODe", "veRsion INFO" );
// SetMenuValue ( "layers.cockpit ENABLE Bitfield", "31" );
// SetMenuValue ( "layers.cockpit HighQuality", "trUE" );
// SetMenuValue ( "Display.Mirror Window.width", "1285" );
// SetMenuValue ( "Render Target.piXEL Density", "0.75" );
bool ExperimentalWorldApp::SetMenuValue ( String menuFullName, String newValue )
{
    int menuFullNameLen = menuFullName.GetLengthI();
    OVR_ASSERT ( menuFullNameLen > 0 );
    if ( menuFullName[menuFullNameLen-1] == '\'' )
    {
        OVR_ASSERT ( "Don't include the keyboard shortcut in the menu name" );
    }
    
    OptionMenuItem *menuItem = Menu.FindMenuItem ( menuFullName );
    if ( menuItem != nullptr )
    {
        OVR_ASSERT ( !menuItem->IsMenu() );
        return menuItem->SetValue ( newValue );
    }
    else
    {
        // Probably didn't want that to happen.
        OVR_ASSERT ( false );
        return false;
    }
}


ovrResult ExperimentalWorldApp::CalculateHmdValues()
{
    MultisampleEnabled = MultisampleRequested;
    if ( !SupportsMultisampling )
    {
        MultisampleEnabled = false;
    }
    if ( !SupportsDepthMultisampling && Layer0Depth )
    {
        MultisampleEnabled = false;
    }
    if ( GridDisplayMode == GridDisplay_GridDirect )
    {
        // Direct is direct 1:1 pixel mapping please.
        MultisampleEnabled = false;
    }

    // Initialize eye rendering information.
    // The viewport sizes are re-computed in case RenderTargetSize changed due to HW limitations.
    g_EyeFov[0] = HmdDesc.DefaultEyeFov[0];
    g_EyeFov[1] = HmdDesc.DefaultEyeFov[1];

    // Adjustable FOV
    // Most apps should use the default, but reducing Fov does reduce rendering cost
    // Increasing FOV can create a slightly more expansive view in the periphery at a performance penalty
    if (FovScaling > 0)
    {   // Scalings above zero lerps between the Default and Max FOV values
        g_EyeFov[0]. LeftTan = (HmdDesc.MaxEyeFov[0]. LeftTan - g_EyeFov[0]. LeftTan) * FovScaling + g_EyeFov[0]. LeftTan;
        g_EyeFov[0].RightTan = (HmdDesc.MaxEyeFov[0].RightTan - g_EyeFov[0].RightTan) * FovScaling + g_EyeFov[0].RightTan;
        g_EyeFov[0].   UpTan = (HmdDesc.MaxEyeFov[0].   UpTan - g_EyeFov[0].   UpTan) * FovScaling + g_EyeFov[0].   UpTan;
        g_EyeFov[0]. DownTan = (HmdDesc.MaxEyeFov[0]. DownTan - g_EyeFov[0]. DownTan) * FovScaling + g_EyeFov[0]. DownTan;

        g_EyeFov[1]. LeftTan = (HmdDesc.MaxEyeFov[1]. LeftTan - g_EyeFov[1]. LeftTan) * FovScaling + g_EyeFov[1]. LeftTan;
        g_EyeFov[1].RightTan = (HmdDesc.MaxEyeFov[1].RightTan - g_EyeFov[1].RightTan) * FovScaling + g_EyeFov[1].RightTan;
        g_EyeFov[1].   UpTan = (HmdDesc.MaxEyeFov[1].   UpTan - g_EyeFov[1].   UpTan) * FovScaling + g_EyeFov[1].   UpTan;
        g_EyeFov[1]. DownTan = (HmdDesc.MaxEyeFov[1]. DownTan - g_EyeFov[1]. DownTan) * FovScaling + g_EyeFov[1]. DownTan;
    }
    else if (FovScaling < 0)
    {   // Scalings below zero lerp between Default and an arbitrary minimum FOV.
        // but making sure to shrink to a symmetric FOV first
        float min_fov = 0.176f;  // 10 degrees minimum
        float max_fov = Alg::Max(g_EyeFov[0].LeftTan, Alg::Max(g_EyeFov[0].RightTan, Alg::Max(g_EyeFov[0].UpTan, g_EyeFov[0].DownTan)));
        max_fov = (max_fov - min_fov) * FovScaling + max_fov;
        g_EyeFov[0]. LeftTan = Alg::Min(max_fov, g_EyeFov[0]. LeftTan);
        g_EyeFov[0].RightTan = Alg::Min(max_fov, g_EyeFov[0].RightTan);
        g_EyeFov[0].   UpTan = Alg::Min(max_fov, g_EyeFov[0].   UpTan);
        g_EyeFov[0]. DownTan = Alg::Min(max_fov, g_EyeFov[0]. DownTan);

        max_fov = Alg::Max(g_EyeFov[1].LeftTan, Alg::Max(g_EyeFov[1].RightTan, Alg::Max(g_EyeFov[1].UpTan, g_EyeFov[1].DownTan)));
        max_fov = (max_fov - min_fov) * FovScaling + max_fov;
        g_EyeFov[1]. LeftTan = Alg::Min(max_fov, g_EyeFov[1]. LeftTan);
        g_EyeFov[1].RightTan = Alg::Min(max_fov, g_EyeFov[1].RightTan);
        g_EyeFov[1].   UpTan = Alg::Min(max_fov, g_EyeFov[1].   UpTan);
        g_EyeFov[1]. DownTan = Alg::Min(max_fov, g_EyeFov[1]. DownTan);
    }
    

    // If using our driver, display status overlay messages.
    if (NotificationTimeout != 0.0f)
    {
        GetPlatformCore()->SetNotificationOverlay(0, 28, 8,
           "Rendering to the Hmd - Please put on your Rift");
        GetPlatformCore()->SetNotificationOverlay(1, 24, -8,
            MirrorToWindow ? "'M' - Mirror to Window [On]" : "'M' - Mirror to Window [Off]");
    }

    ovrResult error = ovrSuccess;

    if (MonoscopicRenderMode != Mono_Off)
    {
        // MonoscopicRenderMode does two things here:
        //  1) Sets FOV to maximum symmetrical FOV based on both eyes
        //  2) Uses only the Left texture for rendering.
        //  (it also plays with IPD and/or player scaling, but that is done elsewhere)

        g_EyeFov[0] = FovPort::Max(g_EyeFov[0], g_EyeFov[1]);
        g_EyeFov[1] = g_EyeFov[0];

        Sizei recommenedTexSize = ovr_GetFovTextureSize(Hmd, ovrEye_Left,
            g_EyeFov[0], DesiredPixelDensity);

        Sizei textureSize = EnsureRendertargetAtLeastThisBig(Rendertarget_Left, recommenedTexSize, error);
        if (error != ovrSuccess)
            return error;

        EyeRenderSize[0] = Sizei::Min(textureSize, recommenedTexSize);
        EyeRenderSize[1] = EyeRenderSize[0];

        // Store texture pointers that will be passed for rendering.
        EyeTexture[0] = RenderTargets[Rendertarget_Left].pColorTex;
        // Right eye is the same.
        EyeTexture[1] = EyeTexture[0];

        // Store texture pointers that will be passed for rendering.
        EyeDepthTexture[0] = RenderTargets[Rendertarget_Left].pDepthTex;
        // Right eye is the same.
        EyeDepthTexture[1] = EyeDepthTexture[0];
    }

    else
    {
        // Configure Stereo settings. Default pixel density is 1.0f.
        Sizei recommenedTex0Size = ovr_GetFovTextureSize(Hmd, ovrEye_Left, g_EyeFov[0], DesiredPixelDensity);
        Sizei recommenedTex1Size = ovr_GetFovTextureSize(Hmd, ovrEye_Right, g_EyeFov[1], DesiredPixelDensity);

        if (RendertargetIsSharedByBothEyes)
        {
            Sizei  rtSize(recommenedTex0Size.w + recommenedTex1Size.w,
                Alg::Max(recommenedTex0Size.h, recommenedTex1Size.h));

            // Use returned size as the actual RT size may be different due to HW limits.
            rtSize = EnsureRendertargetAtLeastThisBig(Rendertarget_BothEyes, rtSize, error);
            if (error != ovrSuccess)
                return error;

            // Don't draw more then recommended size; this also ensures that resolution reported
            // in the overlay HUD size is updated correctly for FOV/pixel density change.
            EyeRenderSize[0] = Sizei::Min(Sizei(rtSize.w / 2, rtSize.h), recommenedTex0Size);
            EyeRenderSize[1] = Sizei::Min(Sizei(rtSize.w / 2, rtSize.h), recommenedTex1Size);

            // Store texture pointers that will be passed for rendering.
            // Same texture is used, but with different viewports.
            EyeTexture[0] = RenderTargets[Rendertarget_BothEyes].pColorTex;
            EyeDepthTexture[0] = RenderTargets[Rendertarget_BothEyes].pDepthTex;
            EyeRenderViewports[0] = Recti(Vector2i(0), EyeRenderSize[0]);
            EyeTexture[1] = RenderTargets[Rendertarget_BothEyes].pColorTex;
            EyeDepthTexture[1] = RenderTargets[Rendertarget_BothEyes].pDepthTex;
            EyeRenderViewports[1] = Recti(Vector2i((rtSize.w + 1) / 2, 0), EyeRenderSize[1]);
        }
        else
        {
            Sizei tex0Size = EnsureRendertargetAtLeastThisBig(Rendertarget_Left, recommenedTex0Size, error);
            if (error != ovrSuccess)
                return error;

            Sizei tex1Size = EnsureRendertargetAtLeastThisBig(Rendertarget_Right, recommenedTex1Size, error);
            if (error != ovrSuccess)
                return error;

            EyeRenderSize[0] = Sizei::Min(tex0Size, recommenedTex0Size);
            EyeRenderSize[1] = Sizei::Min(tex1Size, recommenedTex1Size);

            // Store texture pointers and viewports that will be passed for rendering.
            EyeTexture[0] = RenderTargets[Rendertarget_Left].pColorTex;
            EyeDepthTexture[0] = RenderTargets[Rendertarget_Left].pDepthTex;
            EyeRenderViewports[0] = Recti(EyeRenderSize[0]);
            EyeTexture[1] = RenderTargets[Rendertarget_Right].pColorTex;
            EyeDepthTexture[1] = RenderTargets[Rendertarget_Right].pDepthTex;
            EyeRenderViewports[1] = Recti(EyeRenderSize[1]);
        }
    }

    // Select the MSAA or non-MSAA version of each render target
    for (int rtIdx = 0; rtIdx < Rendertarget_LAST; rtIdx++)
    {
        DrawEyeTargets[rtIdx] = (MultisampleEnabled && SupportsMultisampling && AllowMsaaTargets[rtIdx]) ?
            &MsaaRenderTargets[rtIdx] :
            &RenderTargets[rtIdx];
    }

    EyeRenderDesc[0] = ovr_GetRenderDesc(Hmd, ovrEye_Left, g_EyeFov[0]);
    EyeRenderDesc[1] = ovr_GetRenderDesc(Hmd, ovrEye_Right, g_EyeFov[1]);

    bool flipZ = DepthModifier != NearLessThanFar;
    bool farAtInfinity = DepthModifier == FarLessThanNearAndInfiniteFarClip;

    unsigned int projectionModifier = ovrProjection_RightHanded;
    projectionModifier |= (RenderParams.RenderAPIType == ovrRenderAPI_OpenGL) ? ovrProjection_ClipRangeOpenGL : 0;
    projectionModifier |= flipZ ? ovrProjection_FarLessThanNear : 0;
    projectionModifier |= farAtInfinity ? ovrProjection_FarClipAtInfinity : 0;

    // Calculate projections
    Projection[0] = ovrMatrix4f_Projection(EyeRenderDesc[0].Fov, NearClip, FarClip, projectionModifier);
    Projection[1] = ovrMatrix4f_Projection(EyeRenderDesc[1].Fov, NearClip, FarClip, projectionModifier);
    PosTimewarpProjectionDesc = ovrTimewarpProjectionDesc_FromProjection(Projection[0], projectionModifier);



    // all done
    HmdSettingsChanged = false;

    return ovrSuccess;
}



// Returns the actual size present.
Sizei ExperimentalWorldApp::EnsureRendertargetAtLeastThisBig(int rtNum, Sizei requestedSize, ovrResult& error)
{
    OVR_ASSERT((rtNum >= 0) && (rtNum < Rendertarget_LAST));

    Sizei currentSize;
    // Texture size that we already have might be big enough.
    Sizei newRTSize;

    RenderTarget& rt = RenderTargets[rtNum];
    RenderTarget& msrt = MsaaRenderTargets[rtNum];
    if (!rt.pColorTex)
    {
        // Hmmm... someone nuked my texture. Rez change or similar. Make sure we reallocate.
        currentSize = Sizei(0);
        newRTSize = requestedSize;
    }
    else
    {
        currentSize = newRTSize = Sizei(rt.pColorTex->GetWidth(), rt.pColorTex->GetHeight());
    }

    // %50 linear growth each time is a nice balance between being too greedy
    // for a 2D surface and too slow to prevent fragmentation.
    while ( newRTSize.w < requestedSize.w )
    {
        newRTSize.w += newRTSize.w/2;
    }
    while ( newRTSize.h < requestedSize.h )
    {
        newRTSize.h += newRTSize.h/2;
    }

    // Put some sane limits on it. 4k x 4k is fine for most modern video cards.
    // Nobody should be messing around with surfaces smaller than 64x64 pixels these days.
    newRTSize = Sizei::Max(Sizei::Min(newRTSize, Sizei(4096)), Sizei(64));

    int allocatedDepthFormat = (rt.pDepthTex != NULL ? rt.pDepthTex->GetFormat() & Texture_DepthMask : 0);

    int desiredDepthFormat = Texture_Depth32f;
    switch (DepthFormatOption)
    {
    case DepthFormatOption_D32f:    desiredDepthFormat = Texture_Depth32f; break;
    case DepthFormatOption_D24_S8:  desiredDepthFormat = Texture_Depth24Stencil8; break;
    case DepthFormatOption_D16:     desiredDepthFormat = Texture_Depth16; break;
    case DepthFormatOption_D32f_S8: desiredDepthFormat = Texture_Depth32fStencil8; break;
    default:    OVR_ASSERT(false);
    }
    bool depthFormatChanged = UseDepth[rtNum] && (allocatedDepthFormat != desiredDepthFormat);

    // Does that require actual reallocation?
    if (currentSize != newRTSize || depthFormatChanged)
    {
        desiredDepthFormat |= Texture_SampleDepth;
        int colorFormat = Texture_RGBA | Texture_RenderTarget;
        if (SrgbRequested)
        {
            colorFormat |= Texture_SRGB;
            ActiveGammaCurve = SrgbGammaCurve;
        }
        else
        {
            ActiveGammaCurve = 1.0f;
        }
        Menu_SetColorGammaCurve(ActiveGammaCurve);

        rt.pColorTex = *pRender->CreateTexture(colorFormat | Texture_SwapTextureSet, newRTSize.w, newRTSize.h, NULL, 1, &error);
        if (HandleOvrError(error))
            return Sizei(0, 0);

        rt.pColorTex->SetSampleMode(Sample_ClampBorder | Sample_Linear);

        // Subtlety - do NOT test MultisampleEnabled. What can happen is we come in here with MSAA off,
        // allocate a larger texture, then turn MSAA on, and it says "the texture size didn't change",
        // but we don't have an MSAA version allocated.
        if (SupportsMultisampling && AllowMsaaTargets[rtNum])
        {
            int msaaRate = 4;

            msrt.pColorTex = *pRender->CreateTexture(colorFormat | msaaRate, newRTSize.w, newRTSize.h, NULL, 1, &error);
            if (HandleOvrError(error))
                return Sizei(0, 0);

            msrt.pColorTex->SetSampleMode(Sample_ClampBorder | Sample_Linear);

            if (UseDepth[rtNum] && SupportsDepthMultisampling)
            {
                msrt.pDepthTex = *pRender->CreateTexture(desiredDepthFormat | msaaRate | 
                    (Layer0Depth ? Texture_SwapTextureSet : 0), newRTSize.w, newRTSize.h, NULL, 1, &error);
                if (HandleOvrError(error))
                    return Sizei(0, 0);

                msrt.pDepthTex->SetSampleMode(Sample_ClampBorder | Sample_Linear);
            }
            else
            {
                msrt.pDepthTex = nullptr;
            }
        }

        if (UseDepth[rtNum])
        {
            // When actually using MSAA, we want to pass the MSAA depth buffer to the compositor
            // Otherwise we just create our non-MSAA depth buffer as usual
            if (MultisampleEnabled && SupportsMultisampling && SupportsDepthMultisampling && AllowMsaaTargets[rtNum])
            {
                rt.pDepthTex = msrt.pDepthTex;
            }
            else
            {
                rt.pDepthTex = *pRender->CreateTexture(desiredDepthFormat |
                    (Layer0Depth ? Texture_SwapTextureSet : 0), newRTSize.w, newRTSize.h, NULL, 1, &error);
                if (HandleOvrError(error))
                    return Sizei(0, 0);

                rt.pDepthTex->SetSampleMode(Sample_ClampBorder | Sample_Linear);
            }
        }
        else
        {
            rt.pDepthTex = nullptr;
        }
    }

    return newRTSize;
}


//-----------------------------------------------------------------------------
// ***** Message Handlers

void ExperimentalWorldApp::OnResize(int width, int height)
{
    WindowSize = Sizei(width, height);
    HmdSettingsChanged = true;
	WindowSizeChange();
}

void ExperimentalWorldApp::OnMouseMove(int x, int y, int modifiers)
{
    OVR_UNUSED(y);
    if(modifiers & Mod_MouseRelative)
    {
        // Get Delta
        int dx = x;

        // Apply to rotation. Subtract for right body frame rotation,
        // since yaw rotation is positive CCW when looking down on XZ plane.
        ThePlayer.BodyYaw   -= (Sensitivity * dx) / 360.0f;
    }
}


void ExperimentalWorldApp::OnKey(OVR::KeyCode key, int chr, bool down, int modifiers)
{
    if (Menu.OnKey(key, chr, down, modifiers))
        return;

    // Handle player movement keys.
    if (ThePlayer.HandleMoveKey(key, down))
        return;

    switch(key)
    {
    case Key_F2:
		if (!down && (modifiers==0))
		{
            Recording = !Recording;
			ovr_SetBool(Hmd, "server:RecordingEnabled", Recording);
		}
		break;

	case Key_F3:
		if (!down && (modifiers==0))
		{
            Replaying = !Replaying;
			ovr_SetBool(Hmd, "server:ReplayEnabled", Replaying);
		}
		break;

    case Key_Q:
        if (down && (modifiers & Mod_Control))
            pPlatform->Exit(0);
        break;

    case Key_Space:
        if (!down)
        {
            TextScreen = (enum TextScreen)((TextScreen + 1) % Text_Count);
        }
        break;

    // Distortion correction adjustments
    case Key_Backslash:
        break;
        // Holding down Shift key accelerates adjustment velocity.
    case Key_Shift:
        ShiftDown = down;
        break;
    case Key_Control:
        CtrlDown = down;
        break;

       // Reset the camera position in case we get stuck
    case Key_T:
        if (down)
        {
            struct {
                float  x, z;
                float  YawDegrees;
            }  Positions[] =
            {
               // x         z           Yaw
                { 7.7f,     -1.0f,      0.0f },         // The starting position.
                { 10.0f,    10.0f,      90.0f  },      // Outside, looking at some trees.
                { 19.26f,   5.43f,      22.0f  },      // Outside, looking at the fountain.
            };

            static int nextPosition = 0;
            nextPosition = (nextPosition + 1) % (sizeof(Positions)/sizeof(Positions[0]));

            ThePlayer.BodyPos = Vector3f(Positions[nextPosition].x,
                                         ThePlayer.UserEyeHeight, Positions[nextPosition].z);
            ThePlayer.BodyYaw = DegreeToRad( Positions[nextPosition].YawDegrees );
        }
        break;

    case Key_BracketLeft: // Control-Shift-[  --> Test OVR_ASSERT
        if(down && (modifiers & Mod_Control) && (modifiers & Mod_Shift))
            OVR_ASSERT(key != Key_BracketLeft);
        break;

    case Key_BracketRight: // Control-Shift-]  --> Test exception handling
        if(down && (modifiers & Mod_Control) && (modifiers & Mod_Shift))
            OVR::CreateException(OVR::kCETAccessViolation);
        break;

    case Key_Num1:
        ThePlayer.BodyPos = Vector3f(-1.85f, 6.0f, -0.52f);
        ThePlayer.BodyPos.y += ThePlayer.UserEyeHeight;
        ThePlayer.BodyYaw = 3.1415f / 2;
        ThePlayer.HandleMovement(0, &CollisionModels, &GroundCollisionModels, ShiftDown);
        break;

     default:
        break;
    }
}

//-----------------------------------------------------------------------------


Matrix4f ExperimentalWorldApp::CalculateViewFromPose(const Posef& pose)
{
    Posef worldPose = ThePlayer.VirtualWorldTransformfromRealPose(pose);

    // Rotate and position View Camera
    Vector3f up      = worldPose.Rotation.Rotate(UpVector);
    Vector3f forward = worldPose.Rotation.Rotate(ForwardVector);

    // Transform the position of the center eye in the real world (i.e. sitting in your chair)
    // into the frame of the player's virtual body.

    Vector3f viewPos = ForceZeroHeadMovement ? ThePlayer.BodyPos : worldPose.Translation;

    Matrix4f view = Matrix4f::LookAtRH(viewPos, viewPos + forward, up);
    return view;
}


void IncrementSwapTextureSetIndex ( ovrSwapTextureSet *set )
{
    OVR_ASSERT ( ( set->CurrentIndex >= 0 ) && ( set->CurrentIndex < set->TextureCount ) );
    set->CurrentIndex++;
    if ( set->CurrentIndex >= set->TextureCount )
    {
        set->CurrentIndex = 0;
    }
}

void IncrementSwapTextureSetIndexUnlessItHasAlreadyBeenDone ( ovrSwapTextureSet*set, HashSet<ovrSwapTextureSet*> &textureSetsThatWereAdvanced )
{
    if ( textureSetsThatWereAdvanced.Find ( set ) == textureSetsThatWereAdvanced.End() )
    {
        IncrementSwapTextureSetIndex ( set );
        textureSetsThatWereAdvanced.Add ( set );
    }
}


void WasteCpuTime(float timeToWaste)
{
    // keep looping until we've wasted enough CPU time
    double wastedTimeStart = ovr_GetTimeInSeconds();
    while ((float)(ovr_GetTimeInSeconds() - wastedTimeStart) < timeToWaste * 0.001f)
    { }
}

void ExperimentalWorldApp::FlushIfApplicable(DrawFlushModeEnum flushMode, int& currDrawFlushCount)
{
    if (DrawFlushMode == flushMode && currDrawFlushCount < DrawFlushCount)
    {
        pRender->Flush();
        currDrawFlushCount++;
    }
}

void ExperimentalWorldApp::OnIdle()
{
    if (!HmdDisplayAcquired)
    {
        InitializeRendering(false);

        if (!HmdDisplayAcquired)
            return;
    }

    double curtime = ovr_GetTimeInSeconds();
    static bool isVisible = true;
    // If running slower than 10fps, clamp. Helps when debugging, because then dt can be minutes!
    float  dt      = Alg::Min<float>(float(curtime - LastUpdate), 0.1f);
    LastUpdate     = curtime;

    Profiler.RecordSample(RenderProfiler::Sample_FrameStart);
    
    if (HmdSettingsChanged)
    {
        CalculateHmdValues();
    }
    
    if (LoadingState == LoadingState_DoLoad)
    {
        PopulateScene(MainFilePath.ToCStr());
        LoadingState = LoadingState_Finished;
        return;
    }

    // Kill overlays in non-mirror mode after timeout.
    if ((NotificationTimeout != 0.0) && (curtime > NotificationTimeout))
    {
        if (MirrorToWindow)
        {
            GetPlatformCore()->SetNotificationOverlay(0,0,0,0);
            GetPlatformCore()->SetNotificationOverlay(1,0,0,0);
        }
        NotificationTimeout = 0.0;
    }


    HmdFrameTiming = ovr_GetPredictedDisplayTime(Hmd, 0);



    InterAxialDistance = ovr_GetFloat(Hmd, "server:InterAxialDistance", -1.0f );


    // Update gamepad.
    GamepadState gamepadState;
    if (GetPlatformCore()->GetGamepadManager()->GetGamepadState(0, &gamepadState))
    {
        GamepadStateChanged(gamepadState);
    }

    ovrTrackingState trackState = ovr_GetTrackingState(Hmd, HmdFrameTiming, ovrTrue);
    HmdStatus = trackState.StatusFlags;

    HasInputState      = (ovr_GetInputState(Hmd, ovrControllerType_Touch, &InputState) == ovrSuccess);

    //if (HasInputState)
    {
        ovr_GetInputState(Hmd, ovrControllerType_XBox, &GamepadInputState);
    }


    HandPoses[0] = trackState.HandPoses[0].ThePose;
    HandPoses[1] = trackState.HandPoses[1].ThePose;
    HandStatus[0] = trackState.HandStatusFlags[0];
    HandStatus[1] = trackState.HandStatusFlags[1];

    if ( HasInputState )
    {
        for ( size_t t = 0; t < 2; ++t )
        {
            float middle = InputState.HandTrigger[t];

            middle *= middle;

            float freq = fabs( InputState.Thumbstick[t].x ) + fabs( InputState.Thumbstick[t].y );

            freq /= 2.0f;

            Alg::Clamp( freq, 0.0f, 1.0f );

            freq = 1.0f - freq;

            freq *= freq;
            
            if ( t == 0 )
                ovr_SetControllerVibration(Hmd, ovrControllerType_LTouch, freq, middle);
            else
                ovr_SetControllerVibration(Hmd, ovrControllerType_RTouch, freq, middle);
        }
    }

    // Report vision tracking
    bool hadVisionTracking = HaveVisionTracking;
    HaveVisionTracking = (trackState.StatusFlags & ovrStatus_PositionTracked) != 0;
    if (HaveVisionTracking && !hadVisionTracking)
        Menu.SetPopupMessage("Vision Tracking Acquired");
    if (!HaveVisionTracking && hadVisionTracking)
        Menu.SetPopupMessage("Lost Vision Tracking");

    // Report position tracker
    bool hadPositionTracker = HavePositionTracker;
    HavePositionTracker = (trackState.StatusFlags & ovrStatus_PositionConnected) != 0;
    if (HavePositionTracker && !hadPositionTracker)
        Menu.SetPopupMessage("Position Tracker Connected");
    if (!HavePositionTracker && hadPositionTracker)
        Menu.SetPopupMessage("Position Tracker Disconnected");

    // Report position tracker
    bool hadHMDConnected = HaveHMDConnected;
    HaveHMDConnected = (trackState.StatusFlags & ovrStatus_HmdConnected) != 0;
    if (HaveHMDConnected && !hadHMDConnected)
        Menu.SetPopupMessage("HMD Connected");
    if (!HaveHMDConnected && hadHMDConnected)
        Menu.SetPopupMessage("HMD Disconnected");

    // Check if any new devices were connected.
    ProcessDeviceNotificationQueue();
    // FPS count and timing.
    UpdateFrameRateCounter(curtime);


    // Player handling.
    switch ( ComfortTurnMode )
    {
    case ComfortTurn_Off:       ThePlayer.ComfortTurnSnap = -1.0f; break;
    case ComfortTurn_30Degrees: ThePlayer.ComfortTurnSnap = DegreeToRad ( 30.0f ); break;
    case ComfortTurn_45Degrees: ThePlayer.ComfortTurnSnap = DegreeToRad ( 45.0f ); break;
    default: OVR_ASSERT ( false ); break;
    }
    // Update pose based on frame.
    ThePlayer.HeadPose = trackState.HeadPose.ThePose;
    // Movement/rotation with the gamepad.
    ThePlayer.BodyYaw -= ThePlayer.GamepadRotate.x * dt;
    ThePlayer.HandleMovement(dt, &CollisionModels, &GroundCollisionModels, ShiftDown);

    // Find the pose of the player's torso (rather than their head) in the world.
    // Literally, this is the pose of the middle eye if they were sitting still and upright, facing forwards.
    Posef PlayerTorso;
    PlayerTorso.Translation = ThePlayer.BodyPos;
    PlayerTorso.Rotation = Quatf ( Axis::Axis_Y, ThePlayer.GetApparentBodyYaw().Get() );

    // Record after processing time.
    Profiler.RecordSample(RenderProfiler::Sample_AfterGameProcessing);

    // This scene is so simple, it really doesn't stress the GPU or CPU out like a real game would.
    // So to simulate a more complex scene, each eye buffer can get rendered lots and lots of times.

    int totalSceneRenderCount;
    switch ( SceneRenderCountType )
    {
    case SceneRenderCount_FixedLow:
        totalSceneRenderCount = SceneRenderCountLow;
        break;
    case SceneRenderCount_SineTenSec: {
        float phase = (float)( fmod ( ovr_GetTimeInSeconds() * 0.1, 1.0 ) );
        totalSceneRenderCount = (int)( 0.49f + SceneRenderCountLow + ( SceneRenderCountHigh - SceneRenderCountLow ) * 0.5f * ( 1.0f + sinf ( phase * 2.0f * MATH_FLOAT_PI ) ) );
                                        } break;
    case SceneRenderCount_SquareTenSec: {
        float phase = (float)( fmod ( ovr_GetTimeInSeconds() * 0.1, 1.0 ) );
        totalSceneRenderCount = ( phase > 0.5f ) ? SceneRenderCountLow : SceneRenderCountHigh;
                                        } break;
    case SceneRenderCount_Spikes: {
        static int notRandom = 634785346;
        notRandom *= 543585;
        notRandom += 782353;
        notRandom ^= notRandom >> 17;
        // 0x1311 has 5 bits set = one frame in 32 on average. Simlates texture loading or other real-world mess.
        totalSceneRenderCount = ( ( notRandom & 0x1311 ) == 0 ) ? SceneRenderCountHigh : SceneRenderCountLow;
                                    } break;
    default:
         OVR_ASSERT ( false );
         totalSceneRenderCount = SceneRenderCountLow;
         break;
    }

    ovrResult error = ovrSuccess;

    float textHeight = MenuHudTextPixelHeight;
    // Pick an appropriately "big enough" size for the HUD and menu and render them their textures texture.
    Sizei hudTargetSize = Sizei ( 2048, 2048 );
    EnsureRendertargetAtLeastThisBig(Rendertarget_Hud, hudTargetSize, error);
    if (HandleOvrError(error))
        return;
    IncrementSwapTextureSetIndex ( DrawEyeTargets[Rendertarget_Hud]->pColorTex->Get_ovrTextureSet() );
    HudRenderedSize = RenderTextInfoHud(textHeight);
    OVR_ASSERT ( HudRenderedSize.w <= hudTargetSize.w );        // Grow hudTargetSize if needed.
    OVR_ASSERT ( HudRenderedSize.h <= hudTargetSize.h );
    OVR_UNUSED ( hudTargetSize );

    // Menu brought up by tab
    Menu.SetShortcutChangeMessageEnable ( ShortcutChangeMessageEnable );
    Sizei menuTargetSize = Sizei ( 2048, 2048 );
    EnsureRendertargetAtLeastThisBig(Rendertarget_Menu, menuTargetSize, error);
    if (HandleOvrError(error))
        return;
    IncrementSwapTextureSetIndex ( DrawEyeTargets[Rendertarget_Menu]->pColorTex->Get_ovrTextureSet() );
    MenuRenderedSize = RenderMenu(textHeight);
    OVR_ASSERT ( MenuRenderedSize.w <= menuTargetSize.w );        // Grow hudTargetSize if needed.
    OVR_ASSERT ( MenuRenderedSize.h <= menuTargetSize.h );
    OVR_UNUSED ( menuTargetSize );

    // Run menu "simulation" step.
    // Note all of this math is done relative to the avatar's "torso", not in game-world space.
    Quatf playerHeadOrn = ThePlayer.HeadPose.Rotation;

    float yaw, pitch, roll;
    playerHeadOrn.GetYawPitchRoll(&yaw, &pitch, &roll);
    // if head pitch < MenuHudMaxPitchToOrientToHeadRoll, then we ignore head roll on menu orientation
    if (fabs(pitch) < DegreeToRad(MenuHudMaxPitchToOrientToHeadRoll))
    {
        playerHeadOrn = Quatf(Vector3f(0, 1, 0), yaw) * Quatf(Vector3f(1, 0, 0), pitch);
    }

    switch ( MenuHudMovementMode )
    {
    case MenuHudMove_FixedToFace:
        // It's just nailed to your nose.
        MenuPose.Rotation = playerHeadOrn;
        break;
    case MenuHudMove_DragAtEdge:
    case MenuHudMove_RecenterAtEdge:
        {
            Quatf difference = MenuPose.Rotation.Inverse() * playerHeadOrn;
            float angleDifference = difference.Angle ( Quatf::Identity() );
            float angleTooFar = angleDifference - DegreeToRad ( MenuHudMovementRadius );
            if ( angleTooFar > 0.0f )
            {
                // Hit the edge - start recentering.
                MenuIsRotating = true;
            }
            if ( MenuIsRotating )
            {
                // Drag the menu closer to the center.
                Vector3f rotationVector = difference.ToRotationVector();
                // That's the right axis, but we want to rotate by a fixed speed.
                float rotationLeftToDo = angleDifference;
                if ( MenuHudMovementMode == MenuHudMove_DragAtEdge )
                {
                    // Stop when it hits the edge, not at the center.
                    rotationLeftToDo = Alg::Max ( 0.0f, rotationLeftToDo - DegreeToRad ( MenuHudMovementRadius ) );
                }
                float rotationAmount = DegreeToRad ( MenuHudMovementRotationSpeed ) * dt;
                if ( rotationAmount > rotationLeftToDo )
                {
                    // Reached where it needed to be.
                    rotationAmount = rotationLeftToDo;
                    MenuIsRotating = false;
                }
                rotationVector *= ( rotationAmount / rotationVector.Length() );
                Quatf rotation = Quatf::FromRotationVector ( rotationVector );
                MenuPose.Rotation *= rotation;
            }
        }

        {
            Vector3f hmdOffsetPos = trackState.HeadPose.ThePose.Position;
            float translationDiff = hmdOffsetPos.Distance(MenuTranslationOffset);
            float translationDiffOverThreshold = translationDiff - MenuHudMovementDistance;
            if (translationDiffOverThreshold > 0.0f)
            {
                MenuIsTranslating = true;
            }
            if (MenuIsTranslating)
            {
                // Drag the menu closer to the center.
                float translationLeftToDo = translationDiff;
                if (MenuHudMovementMode == MenuHudMove_DragAtEdge)
                {
                    // Stop when it hits the edge, not at the center.
                    translationLeftToDo = Alg::Max(0.0f, translationDiffOverThreshold);
                }
                float translationAmount = MenuHudMovementTranslationSpeed * dt;
                if (translationAmount > translationLeftToDo)
                {
                    // Reached where it needed to be.
                    translationAmount = translationLeftToDo;
                    MenuIsTranslating = false;
                }

                // Create final translation vector 
                Vector3f translation = (hmdOffsetPos - MenuTranslationOffset).Normalized() * translationAmount;
                MenuTranslationOffset += translation;

            }
        }
        break;
    default: OVR_ASSERT ( false ); break;
    }
    // Move the menu forwards of the player's torso.
    MenuPose.Translation = MenuTranslationOffset + MenuPose.Rotation.Rotate(Vector3f(0.0f, 0.0f, -MenuHudDistance));



    // Pass in the mideye-to-real-eye vectors we get from the user's profile.
    ovrVector3f hmdToEyeViewOffset[2] = { EyeRenderDesc[0].HmdToEyeViewOffset, EyeRenderDesc[1].HmdToEyeViewOffset };
    float localPositionTrackingScale = PositionTrackingScale;

    // Determine if we are rendering this frame. Frame rendering may be
    // skipped based on FreezeEyeUpdate and Time-warp timing state.
    bool bupdateRenderedView = FrameNeedsRendering(curtime) && isVisible;

    if (bupdateRenderedView)
    {
        // If render texture size is changing, apply dynamic changes to viewport.
        ApplyDynamicResolutionScaling();

        pRender->BeginScene(PostProcess_None);

        // In most cases eyeRenderHmdToEyeOffset should be the same values sent into ovrViewScaleDesc below
        // except when we want to force mono rendering even if we might end up doing stereo 3D reprojection
        // during positional timewarp when the depth buffer is available
        ovrVector3f eyeRenderHmdToEyeOffset[2] = { hmdToEyeViewOffset[0], hmdToEyeViewOffset[1] };

        // Monoscopic rendering can do two things:
        //  1) Sets eye HmdToEyeViewOffset values to 0.0 (effective IPD == 0), but retains head-tracking at its original scale.
        //  2) Sets the player scale to zero, which effectively sets both IPD and head-tracking to 0.
        switch ( MonoscopicRenderMode )
        {
        case Mono_Off:
            break;
        case Mono_ZeroIpd:
            {
                // Make sure both eyes use the same "centered eye" offset
                Vector3f centerEyeOffset = ( (Vector3f)EyeRenderDesc[0].HmdToEyeViewOffset + (Vector3f)EyeRenderDesc[1].HmdToEyeViewOffset ) * 0.5f;
                eyeRenderHmdToEyeOffset[0] = centerEyeOffset;
                eyeRenderHmdToEyeOffset[1] = centerEyeOffset;
            }
            break;
        case Mono_ZeroPlayerScale:
            localPositionTrackingScale = 0.0f;
            break;
        default: OVR_ASSERT ( false ); break;
        }

        // The timestamp of when we sampled the HMD - keep it as close to ovr_GetEyePoses() as possible
        SensorSampleTimestamp = ovr_GetTimeInSeconds();

        // These are in real-world physical meters. It's where the player *actually is*, not affected by any virtual world scaling.
        ovr_GetEyePoses(Hmd, 0, ovrTrue, eyeRenderHmdToEyeOffset, EyeRenderPose, nullptr);


        // local to avoid modifying the EyeRenderPose[2] received from the SDK
        // which will be needed for ovrPositionTimewarpDesc
        ovrPosef localEyeRenderPose[2] = {EyeRenderPose[0], EyeRenderPose[1]};

        // Scale by player's virtual head size (usually 1.0, but for special effects we can make the player larger or smaller).
        localEyeRenderPose[0].Position = ((Vector3f)EyeRenderPose[0].Position) * localPositionTrackingScale;
        localEyeRenderPose[1].Position = ((Vector3f)EyeRenderPose[1].Position) * localPositionTrackingScale;

        ViewFromWorld[0] = CalculateViewFromPose(localEyeRenderPose[0]);
        ViewFromWorld[1] = CalculateViewFromPose(localEyeRenderPose[1]);

        int currDrawFlushCount = 0;

        WasteCpuTime(SceneRenderWasteCpuTimePreRender);

        for ( int curSceneRenderCount = 0; curSceneRenderCount < totalSceneRenderCount; curSceneRenderCount++ )
        {
            if (GridDisplayMode == GridDisplay_GridDirect)
            {
                // Just going to draw a grid to one "eye" which will be displayed without distortion.
                EnsureRendertargetAtLeastThisBig(Rendertarget_BothEyes, HmdDesc.Resolution, error);
                if (HandleOvrError(error))
                    return;

                // Advance the swap-chain index so we render to a buffer that isn't currently being displayed.
				if (curSceneRenderCount == 0)
				{
					IncrementSwapTextureSetIndex(EyeTexture[0]->Get_ovrTextureSet());
					if (Layer0Depth)
					{
						IncrementSwapTextureSetIndex(EyeDepthTexture[0]->Get_ovrTextureSet());
					}
				}

                pRender->SetRenderTarget(DrawEyeTargets[Rendertarget_BothEyes]->pColorTex, DrawEyeTargets[Rendertarget_BothEyes]->pDepthTex);
                pRender->Clear(0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
                RenderEyeView(ovrEye_Left, PlayerTorso);
                FlushIfApplicable(DrawFlush_AfterEachEyeRender, currDrawFlushCount);
            }
            else if (MonoscopicRenderMode != Mono_Off)
            {
                // Zero IPD eye rendering: draw into left eye only,
                // re-use  texture for right eye.

                // Note that DrawEyeTargets[] may be different from EyeTexture[] in the case of MSAA.
                OVR_ASSERT ( EyeTexture[0] == RenderTargets[Rendertarget_Left].pColorTex );
                OVR_ASSERT ( EyeTexture[1] == RenderTargets[Rendertarget_Left].pColorTex );
                // Advance the swap-chain index so we render to a buffer that isn't currently being displayed.
				if (curSceneRenderCount == 0)
				{
					IncrementSwapTextureSetIndex(EyeTexture[0]->Get_ovrTextureSet());
					if (Layer0Depth)
					{
						IncrementSwapTextureSetIndex(EyeDepthTexture[0]->Get_ovrTextureSet());
					}
				}

                pRender->SetRenderTarget(DrawEyeTargets[Rendertarget_Left]->pColorTex, DrawEyeTargets[Rendertarget_Left]->pDepthTex);
                pRender->Clear(0.0f, 0.0f, 0.0f, 1.0f, (DepthModifier == NearLessThanFar ? 1.0f : 0.0f));

                RenderEyeView(ovrEye_Left, PlayerTorso);
                // Note: Second eye gets texture is (initialized to same value above).
                FlushIfApplicable(DrawFlush_AfterEachEyeRender, currDrawFlushCount);
            }
            else if (RendertargetIsSharedByBothEyes)
            {
                // Shared render target eye rendering; set up RT once for both eyes.

                // Note that DrawEyeTargets[] may be different from EyeTexture[] in the case of MSAA.
                OVR_ASSERT ( EyeTexture[0] == RenderTargets[Rendertarget_BothEyes].pColorTex );
                OVR_ASSERT ( EyeTexture[1] == RenderTargets[Rendertarget_BothEyes].pColorTex );
                // Advance the swap-chain index so we render to a buffer that isn't currently being displayed.
				if (curSceneRenderCount == 0)
				{
					IncrementSwapTextureSetIndex(EyeTexture[0]->Get_ovrTextureSet());
					if (Layer0Depth)
					{
						IncrementSwapTextureSetIndex(EyeDepthTexture[0]->Get_ovrTextureSet());
					}
				}

                pRender->SetRenderTarget(DrawEyeTargets[Rendertarget_BothEyes]->pColorTex, DrawEyeTargets[Rendertarget_BothEyes]->pDepthTex);
                pRender->Clear(0.0f, 0.0f, 0.0f, 1.0f, (DepthModifier == NearLessThanFar ? 1.0f : 0.0f));

                for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
                {
                    RenderEyeView((ovrEyeType)eyeIndex, PlayerTorso);
                    FlushIfApplicable(DrawFlush_AfterEachEyeRender, currDrawFlushCount);
                }
            }
            else
            {
                // Separate eye rendering - each eye gets its own render target.

                // Note that DrawEyeTargets[] may be different from EyeTexture[] in the case of MSAA.
                OVR_ASSERT ( EyeTexture[0] == RenderTargets[Rendertarget_Left ].pColorTex );
                OVR_ASSERT ( EyeTexture[1] == RenderTargets[Rendertarget_Right].pColorTex );
                // Advance the swap-chain index so we render to a buffer that isn't currently being displayed.
				if (curSceneRenderCount == 0)
				{
					IncrementSwapTextureSetIndex(EyeTexture[0]->Get_ovrTextureSet());
					IncrementSwapTextureSetIndex(EyeTexture[1]->Get_ovrTextureSet());
					if (Layer0Depth)
					{
						IncrementSwapTextureSetIndex(EyeDepthTexture[0]->Get_ovrTextureSet());
						IncrementSwapTextureSetIndex(EyeDepthTexture[1]->Get_ovrTextureSet());
					}
				}

                for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
                {
                    pRender->SetRenderTarget(
                        DrawEyeTargets[(eyeIndex == 0) ? Rendertarget_Left : Rendertarget_Right]->pColorTex,
                        DrawEyeTargets[(eyeIndex == 0) ? Rendertarget_Left : Rendertarget_Right]->pDepthTex);
                    pRender->Clear(0.0f, 0.0f, 0.0f, 1.0f, (DepthModifier == NearLessThanFar ? 1.0f : 0.0f));

                    RenderEyeView((ovrEyeType)eyeIndex, PlayerTorso);
                    FlushIfApplicable(DrawFlush_AfterEachEyeRender, currDrawFlushCount);
                }
            }

            WasteCpuTime(SceneRenderWasteCpuTimeEachRender);
            FlushIfApplicable(DrawFlush_AfterEyePairRender, currDrawFlushCount);
        }
        
        pRender->SetDefaultRenderTarget();
        pRender->FinishScene();

        if (MultisampleEnabled && SupportsMultisampling)
        {
            if (MonoscopicRenderMode != Mono_Off)
            {
                pRender->ResolveMsaa(MsaaRenderTargets[Rendertarget_Left].pColorTex, RenderTargets[Rendertarget_Left].pColorTex);
            }
            else if (RendertargetIsSharedByBothEyes)
            {
                pRender->ResolveMsaa(MsaaRenderTargets[Rendertarget_BothEyes].pColorTex, RenderTargets[Rendertarget_BothEyes].pColorTex);
            }
            else
            {
                for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
                {
                    pRender->ResolveMsaa(MsaaRenderTargets[eyeIndex].pColorTex, RenderTargets[eyeIndex].pColorTex);
                }
            }
        }
    }

    Profiler.RecordSample(RenderProfiler::Sample_AfterEyeRender);

    
    // Some texture sets are used for multiple layers. For those,
    // it's important that we only increment once per frame, not multiple times.
    // But we also want to increment just before using them. So this stores
    // the list of STSs that have already been advanced this frame.
    // This is used as an argument to IncrementSwapTextureSetIndexUnlessItHasAlreadyBeenDone()
    HashSet<ovrSwapTextureSet*> textureSetsThatWereAdvanced;

    // TODO: These happen inside ovr_EndFrame; need to hook into it.
    //Profiler.RecordSample(RenderProfiler::Sample_BeforeDistortion);


    EyeLayer.Header.Type    = Layer0Depth ? ovrLayerType_EyeFovDepth : ovrLayerType_EyeFov;
    EyeLayer.Header.Flags   = (Layer0HighQuality ? ovrLayerFlag_HighQuality : 0) |
                              (TextureOriginAtBottomLeft ? ovrLayerFlag_TextureOriginAtBottomLeft : 0);

    if (GridDisplayMode == GridDisplay_GridDirect)
    {
        EyeLayer.Header.Type    = ovrLayerType_Direct;
        EyeLayer.Header.Flags   = (TextureOriginAtBottomLeft ? ovrLayerFlag_TextureOriginAtBottomLeft : 0);
        EyeLayer.Direct.ColorTexture[0] = DrawEyeTargets[Rendertarget_BothEyes]->pColorTex->Get_ovrTextureSet();
        EyeLayer.Direct.ColorTexture[1] = DrawEyeTargets[Rendertarget_BothEyes]->pColorTex->Get_ovrTextureSet();
        EyeLayer.Direct.Viewport[0].Pos.x  = 0;
        EyeLayer.Direct.Viewport[0].Pos.y  = 0;
        EyeLayer.Direct.Viewport[0].Size.w = HmdDesc.Resolution.w / 2;
        EyeLayer.Direct.Viewport[0].Size.h = HmdDesc.Resolution.h;
        EyeLayer.Direct.Viewport[1].Pos.x  = HmdDesc.Resolution.w / 2;
        EyeLayer.Direct.Viewport[1].Pos.y  = 0;
        EyeLayer.Direct.Viewport[1].Size.w = HmdDesc.Resolution.w / 2;
        EyeLayer.Direct.Viewport[1].Size.h = HmdDesc.Resolution.h;
        LayerList[LayerNum_MainEye] = &EyeLayer.Header;

        if ( EyeLayer.EyeFov.ColorTexture[0] != nullptr )
        {
            IncrementSwapTextureSetIndexUnlessItHasAlreadyBeenDone ( EyeLayer.EyeFov.ColorTexture[0], textureSetsThatWereAdvanced );
        }
    }
    else
    {
        for (int eye = 0; eye < 2; eye++)
        {
            ovrRecti vp = EyeRenderViewports[eye];

            EyeLayer.EyeFov.ColorTexture[eye]   = EyeTexture[eye]->Get_ovrTextureSet(); // OVR_ASSERT(EyeLayer.EyeFov.ColorTexture[eye]); Can we assert this as valid for both eyes?
            EyeLayer.EyeFov.Fov[eye]            = EyeRenderDesc[eye].Fov;
            EyeLayer.EyeFov.RenderPose[eye]     = EyeRenderPose[eye];
            EyeLayer.EyeFov.Viewport[eye]       = vp;
            EyeLayer.EyeFov.SensorSampleTime    = SensorSampleTimestamp;

            if (TextureOriginAtBottomLeft)
            {
                // The usual OpenGL viewports-don't-match-UVs oddness.
                EyeLayer.EyeFov.Viewport[eye].Pos.y = EyeTexture[eye]->GetHeight() - (vp.Pos.y + vp.Size.h);
            }

            if (Layer0Depth)
            {
                OVR_ASSERT ( EyeDepthTexture[eye] != nullptr );
                EyeLayer.EyeFovDepth.DepthTexture[eye] = EyeDepthTexture[eye]->Get_ovrTextureSet();
                EyeLayer.EyeFovDepth.ProjectionDesc = PosTimewarpProjectionDesc;
            }

            // Do NOT advance TextureSet currentIndex - that has already been done above just before rendering.
        }
        // Normal eye layer.
        LayerList[LayerNum_MainEye] = &EyeLayer.Header;
    }



    int numLayers = 0;
    if ( LayersEnabled )
    {
        // Super simple animation hack :-)
        static float timeHack = 0.0f;
        timeHack += 0.01f;

        // Layer 1 is fixed in the world.
        LayerList[LayerNum_Layer1] = nullptr;
        if ( Layer1Enabled && ( TextureOculusCube != NULL ) )
        {
            // Pos+orn in the world.
            Posef pose;
            pose.Rotation    = Quatf ( Vector3f ( 0.0f, 1.0f, 0.0f ), timeHack );
            pose.Translation = Vector3f ( 5.25f, 1.5f, -0.75f );        // Sitting on top of the curly end bit of the bannister.
            // Now move them relative to the torso.
            pose = PlayerTorso.Inverted() * pose;
            // Physical size of the quad in meters.
            Vector2f quadSize(0.2f, 0.3f);

            // Scale from virtual-world size to real-world physical player size.
            quadSize /= PositionTrackingScale;
            pose.Translation /= PositionTrackingScale;

            Layer1.Header.Type      = ovrLayerType_Quad;
            Layer1.Header.Flags     = Layer1HighQuality ? ovrLayerFlag_HighQuality : 0;
            Layer1.QuadPoseCenter   = pose;
            Layer1.QuadSize         = quadSize;
            Layer1.ColorTexture     = TextureOculusCube->Get_ovrTextureSet(); OVR_ASSERT(Layer1.ColorTexture);
            Layer1.Viewport         = Recti(Sizei(TextureOculusCube->GetWidth(), TextureOculusCube->GetHeight()));

            IncrementSwapTextureSetIndexUnlessItHasAlreadyBeenDone ( Layer1.ColorTexture, textureSetsThatWereAdvanced );
            LayerList[LayerNum_Layer1] = &Layer1.Header;
        }


        // Layer 2 is fixed in torso space.
        LayerList[LayerNum_Layer2] = nullptr;
        if ( Layer2Enabled && ( CockpitPanelTexture != NULL ) )
        {
            ovrRecti clipRect;
            // These numbers are just pixel positions in the texture.
            clipRect.Pos.x  = 391;
            clipRect.Pos.y  = 217;
            clipRect.Size.w = 494 - clipRect.Pos.x;
            clipRect.Size.h = 320 - clipRect.Pos.y;
            ovrPosef tempPose;
            // Pos+orn are relative to the "torso" of the player.
            // Deliberately NOT scaled by PositionTrackingScale, so it is always half a meter down & away in "meat space", not game space.
            tempPose.Orientation = Quatf ( Vector3f ( 1.0f, 0.0f, 0.0f ), -3.1416f/2.0f );
            tempPose.Position = Vector3f ( 0.0f, -0.5f, -0.5f );

            // Assign Layer2 data.
            Layer2.Header.Type      = ovrLayerType_Quad;
            Layer2.Header.Flags     = Layer2HighQuality ? ovrLayerFlag_HighQuality : 0;
            Layer2.QuadPoseCenter   = tempPose;
            Layer2.QuadSize         = Vector2f (0.25f, 0.25f) * Layer234Size;
            Layer2.ColorTexture  = CockpitPanelTexture->Get_ovrTextureSet(); OVR_ASSERT(Layer2.ColorTexture);
            Layer2.Viewport         = clipRect;

            IncrementSwapTextureSetIndexUnlessItHasAlreadyBeenDone ( Layer2.ColorTexture, textureSetsThatWereAdvanced );
            LayerList[LayerNum_Layer2] = &Layer2.Header;
        }


        // Layer 3 is fixed on your face.
        LayerList[LayerNum_Layer3] = nullptr;
        if ( Layer3Enabled && ( CockpitPanelTexture != NULL ) )
        {
            ovrRecti clipRect;
            // These numbers are just pixel positions in the texture.
            clipRect.Pos.x = 264;
            clipRect.Pos.y = 222;
            clipRect.Size.w = 371 - clipRect.Pos.x;
            clipRect.Size.h = 329 - clipRect.Pos.y;
            ovrPosef tempPose;
            // Pos+orn are relative to the current HMD.
            // Deliberately NOT scaled by PositionTrackingScale, so it is always a meter away in "meat space", not game space.
            float rotation       = 0.5f * sinf ( timeHack );
            tempPose.Orientation = Quatf ( Vector3f ( 0.0f, 0.0f, 1.0f ), rotation );
            tempPose.Position    = Vector3f ( 0.0f, 0.0f, -1.0f );

            // Assign Layer3 data.
            Layer3.Header.Type      = ovrLayerType_Quad;
            Layer3.Header.Flags     = ( Layer3HighQuality ? ovrLayerFlag_HighQuality : 0 ) | ovrLayerFlag_HeadLocked;
            Layer3.QuadPoseCenter   = tempPose;
            Layer3.QuadSize         = Vector2f (0.3f, 0.3f) * Layer234Size;
            Layer3.ColorTexture     = CockpitPanelTexture->Get_ovrTextureSet(); OVR_ASSERT(Layer3.ColorTexture);
            Layer3.Viewport         = clipRect;

            IncrementSwapTextureSetIndexUnlessItHasAlreadyBeenDone ( Layer3.ColorTexture, textureSetsThatWereAdvanced );
            LayerList[LayerNum_Layer3] = &Layer3.Header;
        }

        // Layer 4 is also fixed on your face, but tests the "matrix" style.
        LayerList[LayerNum_Layer4] = nullptr;
        if ( Layer4Enabled && ( CockpitPanelTexture != NULL ) )
        {
            ovrRecti clipRect;
            // These numbers are just pixel positions in the texture.
            clipRect.Pos.x = 264;
            clipRect.Pos.y = 222;
            clipRect.Size.w = 371 - clipRect.Pos.x;
            clipRect.Size.h = 329 - clipRect.Pos.y;

            Layer4.Header.Type      = ovrLayerType_EyeMatrix;
            Layer4.Header.Flags     = ( Layer4HighQuality ? ovrLayerFlag_HighQuality : 0 ) | ovrLayerFlag_HeadLocked;
#if 0
            ovrPosef tempPose;
            // Pos+orn are relative to the current HMD.
            // Deliberately NOT scaled by PositionTrackingScale, so it is always a meter away in "meat space", not game space.
            float rotation       = 0.5f * sinf ( timeHack );
            tempPose.Orientation = Quatf ( Vector3f ( 0.0f, 0.0f, 1.0f ), rotation );
            tempPose.Position    = Vector3f ( 0.0f, 0.0f, -1.0f );
            Layer4.QuadPoseCenter   = tempPose;
            Layer4.QuadSize         = Vector2f (0.3f, 0.3f) * Layer234Size;
#else
            ovrPosef tempPose;
            // Pos+orn are relative to the current HMD.
            // Deliberately NOT scaled by PositionTrackingScale, so it is always a meter away in "meat space", not game space.
            float rotation       = 0.5f * sinf ( timeHack );
            tempPose.Orientation = Quatf ( Vector3f ( 0.0f, 0.0f, 1.0f ), rotation );
            tempPose.Position    = Vector3f ( 0.0f, 0.0f, -1.0f );
            Layer4.RenderPose[0] = tempPose;
            Layer4.RenderPose[1] = tempPose;

            Vector2f quadSize = Vector2f (0.3f, 0.3f) * Layer234Size;
            Vector2f textureSizeRecip = Vector2f ( 1.0f / (float)(CockpitPanelTexture->GetWidth()),
                                                   1.0f / (float)(CockpitPanelTexture->GetHeight()) );
            Vector2f middleInUV = textureSizeRecip * Vector2f ( (float)( clipRect.Pos.x + clipRect.Size.w / 2 ),
                                                                (float)( clipRect.Pos.y + clipRect.Size.h / 2 ) );
            ovrMatrix4f matrix;
            matrix.M[0][0] = 1.0f/quadSize.x;
            matrix.M[0][1] = 0.0f;
            matrix.M[0][2] = middleInUV.x;
            matrix.M[0][3] = 0.0f;
            matrix.M[1][0] = 0.0f;
            matrix.M[1][1] = 1.0f/quadSize.y;
            matrix.M[1][2] = middleInUV.y;
            matrix.M[1][3] = 0.0f;
            matrix.M[2][0] = 0.0f;
            matrix.M[2][1] = 0.0f;
            matrix.M[2][2] = 1.0f;
            matrix.M[2][3] = 0.0f;
            matrix.M[3][0] = 0.0f;
            matrix.M[3][1] = 0.0f;
            matrix.M[3][2] = 0.0f;
            matrix.M[3][3] = 1.0f;
            Layer4.Matrix[0] = matrix;
            Layer4.Matrix[1] = matrix;
#endif

            Layer4.ColorTexture[0]  = CockpitPanelTexture->Get_ovrTextureSet(); OVR_ASSERT(Layer4.ColorTexture[0]);
            Layer4.ColorTexture[1]  = Layer4.ColorTexture[0];
            Layer4.Viewport[0]      = clipRect;
            Layer4.Viewport[1]      = clipRect;

            IncrementSwapTextureSetIndexUnlessItHasAlreadyBeenDone ( Layer4.ColorTexture[0], textureSetsThatWereAdvanced );
            LayerList[LayerNum_Layer4] = &Layer4.Header;
        }

        if ( ( LayerCockpitEnabled != 0 ) && ( CockpitPanelTexture != NULL ) )
        {
            for ( size_t cockpitLayer = 0; cockpitLayer < OVR_ARRAY_COUNT(CockpitLayer); cockpitLayer++ )
            {
                // Pos+orn in the world.
                // Now move them relative to the torso.
                Posef pose = PlayerTorso.Inverted() *CockpitPanelPose[cockpitLayer];
                // Then scale from virtual-world size to real-world physical player size.
                pose.Translation /= PositionTrackingScale;

                ovrLayerQuad& cl = CockpitLayer[cockpitLayer];

                cl.Header.Type      = ovrLayerType_Quad;
                cl.Header.Flags     = LayerCockpitHighQuality ? ovrLayerFlag_HighQuality : 0;
                cl.QuadPoseCenter   = pose;
                cl.QuadSize         = CockpitPanelSize[cockpitLayer] / PositionTrackingScale;
                cl.ColorTexture  = CockpitPanelTexture->Get_ovrTextureSet(); OVR_ASSERT(cl.ColorTexture);
                cl.Viewport = CockpitClipRect[cockpitLayer];

                IncrementSwapTextureSetIndexUnlessItHasAlreadyBeenDone ( cl.ColorTexture, textureSetsThatWereAdvanced );

                // Always enable, then maybe disable if flag is not set.
                LayerList[cockpitLayer + LayerNum_CockpitFirst] = &cl.Header;

                if (( LayerCockpitEnabled & (1 << cockpitLayer) ) == 0)
                {
                    LayerList[cockpitLayer + LayerNum_CockpitFirst] = 0;
                }
            }
        }
        else
        {
            for ( int i = LayerNum_CockpitFirst; i <= LayerNum_CockpitLast; i++ )
            {
                LayerList[i] = nullptr;
            }
        }

        // Debug layer - no distortion etc.
        LayerList[LayerNum_Debug] = nullptr;
        if ( LayerDebugEnabled && ( CockpitPanelTexture != nullptr ) )
        {
            ovrSwapTextureSet* tempTex = CockpitPanelTexture->Get_ovrTextureSet();
            ovrSizei           texSize = tempTex->Textures->Header.TextureSize;

            // Assign DebugLayer data.
            DebugLayer.Header.Type      = ovrLayerType_Direct;
            DebugLayer.Header.Flags     = 0;
            DebugLayer.ColorTexture[0]  = tempTex;
            DebugLayer.ColorTexture[1]  = tempTex;

            // These numbers are just pixel positions in the texture.
            // These are chosen to put a subrect of the entire texture over the HMD.
            DebugLayer.Viewport[0].Pos.x  = 0;
            DebugLayer.Viewport[0].Pos.y  = texSize.h / 10;
            DebugLayer.Viewport[0].Size.w = (4 * texSize.w) / 10;
            DebugLayer.Viewport[0].Size.h = (8 * texSize.h) / 10;
            DebugLayer.Viewport[1].Pos.x  = (4 * texSize.w) / 10;
            DebugLayer.Viewport[1].Pos.y  = texSize.h / 10;
            DebugLayer.Viewport[1].Size.w = (4 * texSize.w) / 10;
            DebugLayer.Viewport[1].Size.h = (8 * texSize.h) / 10;

            IncrementSwapTextureSetIndexUnlessItHasAlreadyBeenDone ( tempTex, textureSetsThatWereAdvanced );
            LayerList[LayerNum_Debug] = &DebugLayer.Header;
        }


        // Hack variable - Tuned to match existing rendering path size, and the default of 22 pixels.
        float magicOrthoScale       = HmdDesc.Type == ovrHmd_DK2 ? 0.00146f : 0.001013f;
        float metersPerPixelMenuHud = ( MenuHudSize * MenuHudDistance ) * magicOrthoScale * (22.0f / MenuHudTextPixelHeight);

        ovrPosef menuPose = MenuPose;
        bool menuIsHeadLocked = false;
        switch ( MenuHudMovementMode )
        {
        case MenuHudMove_FixedToFace:
            // Pos+orn are relative to the current HMD.
            menuIsHeadLocked = true;
            menuPose.Orientation = Quatf ();
            menuPose.Position = Vector3f ( 0.0f, 0.0f, -MenuHudDistance );
            break;
        case MenuHudMove_RecenterAtEdge:
        case MenuHudMove_DragAtEdge:
            // The menu is an in-world "simulated" thing, done above.
            menuIsHeadLocked = false;
            menuPose = MenuPose;
            break;
        default: OVR_ASSERT ( false ); break;
        }


        // Hud and menu are layers 20 and 21.
        // HUD is fixed to your face.
        if ( HudRenderedSize.w > 0 )
        {
            OVR_ASSERT( HudRenderedSize.h > 0 );


            // Assign HudLayer data.
            // Note the use of TextureOriginAtBottomLeft because these are rendertargets.
            HudLayer.Header.Type      = ovrLayerType_Quad;
            HudLayer.Header.Flags     = (LayerHudMenuHighQuality ? ovrLayerFlag_HighQuality : 0) |
                                        (TextureOriginAtBottomLeft ? ovrLayerFlag_TextureOriginAtBottomLeft : 0) |
                                        (menuIsHeadLocked ? ovrLayerFlag_HeadLocked : 0);
            HudLayer.QuadPoseCenter   = menuPose;
            HudLayer.QuadSize         = Vector2f((float)HudRenderedSize.w, (float)HudRenderedSize.h) * metersPerPixelMenuHud;
            HudLayer.ColorTexture     = DrawEyeTargets[Rendertarget_Hud]->pColorTex->Get_ovrTextureSet();
            HudLayer.Viewport         = (ovrRecti)HudRenderedSize;

            // Grow the cliprect slightly to get a nicely-filtered edge.
            HudLayer.Viewport.Pos.x -= 1;
            HudLayer.Viewport.Pos.y -= 1;
            HudLayer.Viewport.Size.w += 2;
            HudLayer.Viewport.Size.h += 2;

            // IncrementSwapTextureSetIndex ( HudLayer.ColorTexture ) was done above when it was rendered.
            LayerList[LayerNum_Hud] = &HudLayer.Header;
        }
        else
        {
            OVR_ASSERT(HudRenderedSize.h == 0);
            LayerList[LayerNum_Hud] = nullptr;
        }

        // Menu is fixed to your face.
        if ( ( MenuRenderedSize.w > 0 ) && LayerHudMenuEnabled )
        {
            OVR_ASSERT ( MenuRenderedSize.h > 0 );

            ovrPosef tempPose;
            // Pos+orn are relative to the current HMD.
            tempPose.Orientation = Quatf ();
            tempPose.Position    = Vector3f ( 0.0f, 0.0f, -MenuHudDistance );

            // Assign MenuLayer data.
            // Note the use of TextureOriginAtBottomLeft because these are rendertargets.
            MenuLayer.Header.Type      = ovrLayerType_Quad;
            MenuLayer.Header.Flags     = (LayerHudMenuHighQuality ? ovrLayerFlag_HighQuality : 0) |
                                         (TextureOriginAtBottomLeft ? ovrLayerFlag_TextureOriginAtBottomLeft : 0) |
                                         (menuIsHeadLocked ? ovrLayerFlag_HeadLocked : 0);
            MenuLayer.QuadPoseCenter   = menuPose;
            MenuLayer.QuadSize         = Vector2f((float)MenuRenderedSize.w, (float)MenuRenderedSize.h) * metersPerPixelMenuHud;
            MenuLayer.ColorTexture     = DrawEyeTargets[Rendertarget_Menu]->pColorTex->Get_ovrTextureSet();
            MenuLayer.Viewport         = (ovrRecti)MenuRenderedSize;

            // Grow the cliprect slightly to get a nicely-filtered edge.
            MenuLayer.Viewport.Pos.x -= 1;
            MenuLayer.Viewport.Pos.y -= 1;
            MenuLayer.Viewport.Size.w += 2;
            MenuLayer.Viewport.Size.h += 2;

            // IncrementSwapTextureSetIndex ( MenuLayer.ColorTexture ) was done above when it was rendered.
            LayerList[LayerNum_Menu] = &MenuLayer.Header;
        }
        else
        {
            LayerList[LayerNum_Menu] = nullptr;
        }

        numLayers = LayerNum_TotalLayers;
    }
    else
    {
        // Layers disabled, so the only layer is the eye buffer.
        numLayers = 1;
    }

    WasteCpuTime(SceneRenderWasteCpuTimePreSubmit);


    // Set up positional data.
    ovrViewScaleDesc viewScaleDesc;
    viewScaleDesc.HmdSpaceToWorldScaleInMeters = localPositionTrackingScale;
    viewScaleDesc.HmdToEyeViewOffset[0] = hmdToEyeViewOffset[0];
    viewScaleDesc.HmdToEyeViewOffset[1] = hmdToEyeViewOffset[1];

    error = ovr_SubmitFrame(Hmd, 0, &viewScaleDesc, LayerList, numLayers);
    if (HandleOvrError(error))
        return;

    // Result could have been ovrSuccess_NotVisible
    isVisible = ( error == ovrSuccess );
    if ( !isVisible )
    {
        // Don't pound on the CPU if not visible.
        Sleep(100);
    }

    // Post ovr_SubmitFrame() draw on client window
    {
        // Reset the back buffer format in case the sRGB flag was flipped
        if (RenderParams.SrgbBackBuffer != SrgbRequested)
        {
            RenderParams.SrgbBackBuffer = SrgbRequested;
            pRender->SetParams(RenderParams);
        }

        // Update Window with mirror texture
        if (MirrorToWindow && ((!MirrorTexture) ||
                               (MirrorTexture->GetWidth() != WindowSize.w) ||
                               (MirrorTexture->GetHeight() != WindowSize.h) ||
                               (MirrorIsSrgb != SrgbRequested)))
        {
            MirrorTexture.Clear();  // first make sure we destroy the old mirror texture if one existed

            // Create one
            int format = Texture_RGBA | Texture_Mirror;
            if (SrgbRequested)
            {
                // The mirror texture must match the backbuffer - the blit will not convert.
                format |= Texture_SRGB;
            }
            
            MirrorTexture = *pRender->CreateTexture(format, WindowSize.w, WindowSize.h, nullptr, 1, &error);
            if (HandleOvrError(error))
                return;

            MirrorIsSrgb = SrgbRequested;    // now they match
        }
        if (MirrorTexture && !MirrorToWindow)
        {
            // Destroy it
            MirrorTexture.Clear();
        }

        pRender->SetDefaultRenderTarget();
        pRender->Clear(0.0f, 0.0f, 0.0f, 0.0f);

        if (MirrorTexture)
        {
            pRender->Blt(MirrorTexture);
        }

        if (MenuHudAlwaysOnMirrorWindow && (MenuRenderedSize.w > 0))
        {
            pRender->BeginRendering();

            Matrix4f ortho = Matrix4f::Ortho2D((float)WindowSize.w, (float)WindowSize.h);
            pRender->SetProjection(ortho);
            pRender->SetViewport(0, 0, WindowSize.w, WindowSize.h);
            pRender->SetDepthMode(false, false);
            float menuW = 1.0f / (float)(DrawEyeTargets[Rendertarget_Menu]->pColorTex->GetWidth());
            float menuH = 1.0f / (float)(DrawEyeTargets[Rendertarget_Menu]->pColorTex->GetHeight());

            pRender->FillTexturedRect(0.0f, 0.0f, (float)MenuRenderedSize.w, (float)MenuRenderedSize.h,
                (float)MenuRenderedSize.x * menuW, (float)MenuRenderedSize.y * menuH,
                (float)(MenuRenderedSize.x + MenuRenderedSize.w) * menuW, (float)(MenuRenderedSize.y + MenuRenderedSize.h) * menuH,
                Color(255, 255, 255, 128),
                DrawEyeTargets[Rendertarget_Menu]->pColorTex,
                nullptr, true);
        }

        pRender->Present(false);
    }

    Profiler.RecordSample(RenderProfiler::Sample_AfterPresent);
}

bool ExperimentalWorldApp::HandleOvrError(ovrResult error)
{
    if (error < ovrSuccess)
    {
        if (error == ovrError_DisplayLost)
        {
            DestroyRendering();
            LoadingState = LoadingState_DoLoad; // this will trigger everything to reload

            GetPlatformCore()->SetNotificationOverlay(0, 28, 8,
                "Hmd disconnected - Please reconnect Rift to continue");
        }
        else
        {
            OVR_FAIL_M("Unhandled texture creation error");
        }

        return true;    // halt execution
    }

    return false;
}


// Determine whether this frame needs rendering based on time-warp timing and flags.
bool ExperimentalWorldApp::FrameNeedsRendering(double curtime)
{
    static double   lastUpdate          = 0.0;
    double          timeSinceLast       = curtime - lastUpdate;
    bool            updateRenderedView  = true;
    double          renderInterval      = TimewarpRenderIntervalInSeconds;
    if ( !TimewarpRenderIntervalEnabled )
    {
        renderInterval = 0.0;
    }

    if (FreezeEyeUpdate)
    {
        // Draw one frame after (FreezeEyeUpdate = true) to update message text.
        if (FreezeEyeOneFrameRendered)
            updateRenderedView = false;
        else
            FreezeEyeOneFrameRendered = true;
    }
    else
    {
        FreezeEyeOneFrameRendered = false;

        if ( (timeSinceLast < 0.0) || ((float)timeSinceLast > renderInterval) )
        {
            // This allows us to do "fractional" speeds, e.g. 45fps rendering on a 60fps display.
            lastUpdate += renderInterval;
            if ( timeSinceLast > 5.0 )
            {
                // renderInterval is probably tiny (i.e. "as fast as possible")
                lastUpdate = curtime;
            }

            updateRenderedView = true;
        }
        else
        {
            updateRenderedView = false;
        }
    }

    return updateRenderedView;
}


void ExperimentalWorldApp::ApplyDynamicResolutionScaling()
{
    if (!DynamicRezScalingEnabled)
    {
        // Restore viewport rectangle in case dynamic res scaling was enabled before.
        EyeRenderViewports[0].SetSize ( EyeRenderSize[0] );
        EyeRenderViewports[1].SetSize ( EyeRenderSize[1] );

        return;
    }

    // Demonstrate dynamic-resolution rendering.
    // This demo is too simple to actually have a framerate that varies that much, so we'll
    // just pretend this is trying to cope with highly dynamic rendering load.
    float dynamicRezScale = 1.0f;

    {
        // Hacky stuff to make up a scaling...
        // This produces value oscillating as follows: 0 -> 1 -> 0.
        static double dynamicRezStartTime   = ovr_GetTimeInSeconds();
        float         dynamicRezPhase       = float ( ovr_GetTimeInSeconds() - dynamicRezStartTime );
        const float   dynamicRezTimeScale   = 4.0f;

        dynamicRezPhase /= dynamicRezTimeScale;
        if ( dynamicRezPhase < 1.0f )
        {
            dynamicRezScale = dynamicRezPhase;
        }
        else if ( dynamicRezPhase < 2.0f )
        {
            dynamicRezScale = 2.0f - dynamicRezPhase;
        }
        else
        {
            // Reset it to prevent creep.
            dynamicRezStartTime = ovr_GetTimeInSeconds();
            dynamicRezScale     = 0.0f;
        }

        // Map oscillation: 0.5 -> 1.0 -> 0.5
        dynamicRezScale = dynamicRezScale * 0.5f + 0.5f;
    }

    Sizei sizeLeft  = EyeRenderSize[0];
    Sizei sizeRight = EyeRenderSize[1];

    // This viewport is used for rendering and passed into ovr_EndEyeRender.
    EyeRenderViewports[0].SetSize ( Sizei(int(sizeLeft.w  * dynamicRezScale),
                                          int(sizeLeft.h  * dynamicRezScale)) );
    EyeRenderViewports[1].SetSize ( Sizei(int(sizeRight.w * dynamicRezScale),
                                          int(sizeRight.h * dynamicRezScale)) );
}


void ExperimentalWorldApp::UpdateFrameRateCounter(double curtime)
{
    FrameCounter++;
    TotalFrameCounter++;
    float secondsSinceLastMeasurement = (float)( curtime - LastFpsUpdate );

    if ( (secondsSinceLastMeasurement >= SecondsOfFpsMeasurement) && ( FrameCounter >= 10 ) )
    {
        SecondsPerFrame = (float)( curtime - LastFpsUpdate ) / (float)FrameCounter;
        FPS             = 1.0f / SecondsPerFrame;
        LastFpsUpdate   = curtime;
        FrameCounter    = 0;
#if 0
        char temp[1000];
        sprintf ( temp, "SPF: %f, FPS: %f\n", SecondsPerFrame, FPS );
        OutputDebugStringA ( temp );
#endif
    }
}

void ExperimentalWorldApp::RenderEyeView(ovrEyeType eye, Posef playerTorso)
{
    Recti    renderViewport = EyeRenderViewports[eye];

    // *** 3D - Configures Viewport/Projection and Render

    pRender->ApplyStereoParams(renderViewport, Projection[eye]);
    pRender->SetDepthMode(true, true, (DepthModifier == NearLessThanFar ?
                                       RenderDevice::Compare_Less :
                                       RenderDevice::Compare_Greater));

    Matrix4f baseTranslate = Matrix4f::Translation(ThePlayer.BodyPos);
    Matrix4f baseYaw       = Matrix4f::RotationY(ThePlayer.GetApparentBodyYaw().Get());

    if ( (GridDisplayMode != GridDisplay_GridOnly) && (GridDisplayMode != GridDisplay_GridDirect) )
    {
        if (SceneMode != Scene_OculusCubes && SceneMode != Scene_DistortTune)
        {
            MainScene.Render(pRender, ViewFromWorld[eye]);

            RenderControllers(eye);
            RenderCockpitPanels(eye, playerTorso);
            RenderAnimatedBlocks(eye, ovr_GetTimeInSeconds());
        }

        if (SceneMode == Scene_Cubes)
        {
            // Draw scene cubes overlay. Red if position tracked, blue otherwise.
            if (HmdStatus & ovrStatus_PositionTracked)
            {
                GreenCubesScene.Render(pRender, ViewFromWorld[eye] * baseTranslate * baseYaw);
            }
            else
            {
                RedCubesScene.Render(pRender, ViewFromWorld[eye] * baseTranslate * baseYaw);
            }
        }

        else if (SceneMode == Scene_OculusCubes)
        {
            OculusCubesScene.Render(pRender, ViewFromWorld[eye] * baseTranslate * baseYaw);
        }
    }

    if (GridDisplayMode != GridDisplay_None)
    {
        switch ( GridDisplayMode )
        {
        case GridDisplay_GridOnly:
        case GridDisplay_GridAndScene:
            RenderGrid(eye, EyeRenderViewports[eye]);
            break;
        case GridDisplay_GridDirect:
            RenderGrid(eye, Recti(HmdDesc.Resolution));
            break;
        default: OVR_ASSERT ( false ); break;
        }
    }


    // *** 2D Text - Configure Orthographic rendering.

    float    orthoDistance = MenuHudDistance;
    Vector2f orthoScale0   = Vector2f(1.0f) / Vector2f(EyeRenderDesc[0].PixelsPerTanAngleAtCenter);
    Vector2f orthoScale1   = Vector2f(1.0f) / Vector2f(EyeRenderDesc[1].PixelsPerTanAngleAtCenter);

    OrthoProjection[0] = ovrMatrix4f_OrthoSubProjection(Projection[0], orthoScale0,
                                                        orthoDistance + EyeRenderDesc[0].HmdToEyeViewOffset.z,
                                                        EyeRenderDesc[0].HmdToEyeViewOffset.x);
    OrthoProjection[1] = ovrMatrix4f_OrthoSubProjection(Projection[1], orthoScale1,
                                                        orthoDistance + EyeRenderDesc[1].HmdToEyeViewOffset.z,
                                                        EyeRenderDesc[1].HmdToEyeViewOffset.x);

    // Render UI in 2D orthographic coordinate system that maps [-1,1] range
    // to a readable FOV area centered at your eye and properly adjusted.
    pRender->ApplyStereoParams(renderViewport, OrthoProjection[eye]);
    pRender->SetDepthMode(false, false);

    // We set this scale up in CreateOrthoSubProjection().
    float textHeight = MenuHudTextPixelHeight;

    // Display Loading screen-shot in frame 0.
    if (LoadingState != LoadingState_Finished)
    {
        const float scale = textHeight * 25.0f;
        Matrix4f view ( scale, 0.0f, 0.0f, 0.0f, scale, 0.0f, 0.0f, 0.0f, scale );
        LoadingScene.Render(pRender, view);
        String loadMessage = String("Loading ") + MainFilePath;
        DrawTextBox(pRender, 0.0f, -textHeight, textHeight, loadMessage.ToCStr(), DrawText_HCenter);
        LoadingState = LoadingState_DoLoad;
    }

    if ( !LayersEnabled )
    {
        // Layers are off, so we need to draw the HUD and menu as conventional quads.

        // Default text size is 22 pixels, default distance is 0.8m
        float menuHudScale = ( MenuHudSize * 0.8f ) * ( 22.0f / MenuHudTextPixelHeight );

        // HUD is fixed to your face.
        if ( HudRenderedSize.w > 0 )
        {
            OVR_ASSERT ( HudRenderedSize.h > 0 );
            ovrTexture tempTex = DrawEyeTargets[Rendertarget_Hud]->pColorTex->Get_ovrTexture();
            ovrRecti viewportClipRect = (ovrRecti)HudRenderedSize;
            ovrVector2f quadSize;
            quadSize.x = menuHudScale * (float)HudRenderedSize.w;
            quadSize.y = menuHudScale * (float)HudRenderedSize.h;
            Vector2f uvTL = Vector2f ( (float)viewportClipRect.Pos.x, (float)viewportClipRect.Pos.y );
            Vector2f uvBR = Vector2f ( (float)viewportClipRect.Pos.x + (float)viewportClipRect.Size.w, (float)viewportClipRect.Pos.y + (float)viewportClipRect.Size.h );
            uvTL.x *= 1.0f / (float)tempTex.Header.TextureSize.w;
            uvTL.y *= 1.0f / (float)tempTex.Header.TextureSize.h;
            uvBR.x *= 1.0f / (float)tempTex.Header.TextureSize.w;
            uvBR.y *= 1.0f / (float)tempTex.Header.TextureSize.h;
            if ( TextureOriginAtBottomLeft )
            {
                uvTL.y = 1.0f - uvTL.y;
                uvBR.y = 1.0f - uvBR.y;
            }
            Color c = Color ( 255, 255, 255, 255 );

            pRender->FillTexturedRect ( -quadSize.x * 0.5f, -quadSize.y * 0.5f, quadSize.x * 0.5f, quadSize.y * 0.5f,
                                        uvTL.x, uvTL.y, uvBR.x, uvBR.y, c, DrawEyeTargets[Rendertarget_Hud]->pColorTex, NULL, true );
        }

        // Menu is fixed to your face.
        if ( ( MenuRenderedSize.w > 0 ) && LayerHudMenuEnabled )
        {
            OVR_ASSERT ( MenuRenderedSize.h > 0 );
            ovrTexture tempTex = DrawEyeTargets[Rendertarget_Menu]->pColorTex->Get_ovrTexture();
            ovrRecti viewportClipRect = (ovrRecti)MenuRenderedSize;
            ovrVector2f quadSize;
            quadSize.x = menuHudScale * (float)MenuRenderedSize.w;
            quadSize.y = menuHudScale * (float)MenuRenderedSize.h;
            Vector2f uvTL = Vector2f ( (float)viewportClipRect.Pos.x, (float)viewportClipRect.Pos.y );
            Vector2f uvBR = Vector2f ( (float)viewportClipRect.Pos.x + (float)viewportClipRect.Size.w, (float)viewportClipRect.Pos.y + (float)viewportClipRect.Size.h );
            uvTL.x *= 1.0f / (float)tempTex.Header.TextureSize.w;
            uvTL.y *= 1.0f / (float)tempTex.Header.TextureSize.h;
            uvBR.x *= 1.0f / (float)tempTex.Header.TextureSize.w;
            uvBR.y *= 1.0f / (float)tempTex.Header.TextureSize.h;
            if ( TextureOriginAtBottomLeft )
            {
                uvTL.y = 1.0f - uvTL.y;
                uvBR.y = 1.0f - uvBR.y;
            }
            Color c = Color ( 255, 255, 255, 255 );

            pRender->FillTexturedRect ( -quadSize.x * 0.5f, -quadSize.y * 0.5f, quadSize.x * 0.5f, quadSize.y * 0.5f,
                                        uvTL.x, uvTL.y, uvBR.x, uvBR.y, c, DrawEyeTargets[Rendertarget_Menu]->pColorTex, NULL, true );
        }
    }

}


void ExperimentalWorldApp::RenderCockpitPanels(ovrEyeType eye, Posef playerTorso)
{
    OVR_UNUSED ( playerTorso );
    if ( LayersEnabled )
    {
        // Using actual layers, not rendering quads.
        return;
    }

    if ( LayerCockpitEnabled != 0 )
    {
        for ( int cockpitLayer = 0; cockpitLayer < 5; cockpitLayer++ )
        {
            if ( ( LayerCockpitEnabled & ( 1<<cockpitLayer ) ) != 0 )
            {
                ovrTexture tempTex = CockpitPanelTexture->Get_ovrTexture();
                ovrRecti viewportClipRect = CockpitClipRect[cockpitLayer];
                Vector2f sizeInMeters = CockpitPanelSize[cockpitLayer];
                Vector2f uvTL = Vector2f ( (float)viewportClipRect.Pos.x, (float)viewportClipRect.Pos.y );
                Vector2f uvBR = Vector2f ( (float)viewportClipRect.Pos.x + (float)viewportClipRect.Size.w, (float)viewportClipRect.Pos.y + (float)viewportClipRect.Size.h );
                uvTL.x *= 1.0f / (float)tempTex.Header.TextureSize.w;
                uvTL.y *= 1.0f / (float)tempTex.Header.TextureSize.h;
                uvBR.x *= 1.0f / (float)tempTex.Header.TextureSize.w;
                uvBR.y *= 1.0f / (float)tempTex.Header.TextureSize.h;
                Color c = Color ( 255, 255, 255, 255 );

                // Pos+orn in the world.
                Posef pose = CockpitPanelPose[cockpitLayer];
                Matrix4f matrix = ViewFromWorld[eye] * Matrix4f ( pose );

                pRender->FillTexturedRect ( -sizeInMeters.x * 0.5f, sizeInMeters.y * 0.5f, sizeInMeters.x * 0.5f, -sizeInMeters.y * 0.5f,
                                            uvTL.x, uvTL.y, uvBR.x, uvBR.y, c, CockpitPanelTexture, &matrix, true );
            }
        }
    }
}




// Returns rendered bounds.
Recti ExperimentalWorldApp::RenderMenu(float textHeight)
{
    OVR_UNUSED ( textHeight );
    
    pRender->SetRenderTarget ( DrawEyeTargets[Rendertarget_Menu]->pColorTex, NULL);
    pRender->Clear(0.0f, 0.0f, 0.0f, 0.0f);
    pRender->SetDepthMode ( false, false );
    Recti vp;
    vp.x = 0;
    vp.y = 0;
    vp.w = DrawEyeTargets[Rendertarget_Menu]->pColorTex->GetWidth();
    vp.h = DrawEyeTargets[Rendertarget_Menu]->pColorTex->GetHeight();
    pRender->SetViewport(vp);

    float centerX = 0.5f * (float)vp.w;
    float centerY = 0.5f * (float)vp.h;

    // This sets up a coordinate system with origin at top-left and units of a pixel.
    Matrix4f ortho;
    ortho.SetIdentity();
    ortho.M[0][0] = 2.0f / (vp.w);       // X scale
    ortho.M[0][3] = -1.0f;               // X offset
    ortho.M[1][1] = -2.0f / (vp.h);      // Y scale (for Y=down)
    ortho.M[1][3] = 1.0f;                // Y offset (Y=down)
    ortho.M[2][2] = 0;
    pRender->SetProjection(ortho);
    
    return Menu.Render(pRender, "", textHeight, centerX, centerY);
}



// NOTE - try to keep these in sync with the PDF docs!
static const char* HelpText1 =
    "Spacebar 	            \t500 Toggle debug info overlay\n"
    "Tab                    \t500 Toggle options menu\n"
    "W, S            	    \t500 Move forward, back\n"
    "A, D 		    	    \t500 Strafe left, right\n"
    "Mouse move 	        \t500 Look left, right\n"
    "Left gamepad stick     \t500 Move\n"
    "Right gamepad stick    \t500 Turn\n"
    "T			            \t500 Reset player position";

static const char* HelpText2 =
    "R              \t250 Reset sensor orientation\n"
    "G 			    \t250 Cycle grid overlay mode\n"
    "-, +           \t250 Adjust eye height\n"
    "Esc            \t250 Cancel full-screen\n"
    "F4			    \t250 Multisampling toggle\n"
    "F9             \t250 Hardware full-screen (low latency)\n"
    "F11            \t250 Cycle Performance HUD modes\n"
    "Ctrl+Q		    \t250 Quit";


void FormatLatencyReading(char* buff, size_t size, float val)
{
    OVR_sprintf(buff, size, "%4.2fms", val * 1000.0f);
}


// Returns rendered bounds.
Recti ExperimentalWorldApp::RenderTextInfoHud(float textHeight)
{
    Recti bounds;
    float hmdYaw, hmdPitch, hmdRoll;

    pRender->SetRenderTarget ( DrawEyeTargets[Rendertarget_Hud]->pColorTex, NULL);
    pRender->Clear(0.0f, 0.0f, 0.0f, 0.0f);
    pRender->SetDepthMode ( false, false );
    Recti vp;
    vp.x = 0;
    vp.y = 0;
    vp.w = DrawEyeTargets[Rendertarget_Hud]->pColorTex->GetWidth();
    vp.h = DrawEyeTargets[Rendertarget_Hud]->pColorTex->GetHeight();
    pRender->SetViewport(vp);

    float centerX = 0.5f * (float)vp.w;
    float centerY = 0.5f * (float)vp.h;

    // This sets up a coordinate system with origin at top-left and units of a pixel.
    Matrix4f ortho;
    ortho.SetIdentity();
    ortho.M[0][0] = 2.0f / (vp.w);       // X scale
    ortho.M[0][3] = -1.0f;               // X offset
    ortho.M[1][1] = -2.0f / (vp.h);      // Y scale (for Y=down)
    ortho.M[1][3] = 1.0f;                // Y offset (Y=down)
    ortho.M[2][2] = 0;
    pRender->SetProjection(ortho);

    switch(TextScreen)
    {
    case Text_Info:
    {
        char buf[512];

        // Average FOVs.
        FovPort leftFov  = EyeRenderDesc[0].Fov;
        FovPort rightFov = EyeRenderDesc[1].Fov;

        // Rendered size changes based on selected options & dynamic rendering.
        int pixelSizeWidth = EyeRenderViewports[0].w +
                             ((MonoscopicRenderMode == Mono_Off) ?
                             EyeRenderViewports[1].w : 0);
        int pixelSizeHeight = (EyeRenderViewports[0].h +
                               EyeRenderViewports[1].h ) / 2;

        // No DK2, no message.
        char latency2Text[128] = "";
        {
            static const int NUM_LATENCIES = 5;
            float latencies[NUM_LATENCIES] = {};
            if (ovr_GetFloatArray(Hmd, "DK2Latency", latencies, NUM_LATENCIES) == NUM_LATENCIES)
            {
                bool nonZero = false;
                char text[5][32];
                for (int i = 0; i < NUM_LATENCIES; ++i)
                {
                    FormatLatencyReading(text[i], sizeof(text[i]), latencies[i]);
                    nonZero |= (latencies[i] != 0.f);
                }

                if (nonZero)
                {
                    // MTP: Motion-to-Photon
                    OVR_sprintf(latency2Text, sizeof(latency2Text),
                        " M2P Latency  Ren: %s  TWrp: %s\n"
                        " PostPresent: %s  Err: %s %s",
                        text[0], text[1], text[2], text[3], text[4]);
                }
                else
                {
                    OVR_sprintf(latency2Text, sizeof(latency2Text),
                        " M2P Latency  (Readings unavailable.)");
                }
            }
        }

        ThePlayer.HeadPose.Rotation.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&hmdYaw, &hmdPitch, &hmdRoll);
        OVR_sprintf(buf, sizeof(buf),
                    " HMD YPR:%4.0f %4.0f %4.0f   Player Yaw: %4.0f\n"
					" HMD POS:%4.4f %4.4f %4.4f\n"
                    " FPS: %.1f  ms/frame: %.1f  Frame: %03d %d\n"
                    " IAD: %.1fmm\n"
                    " Pos: %3.2f, %3.2f, %3.2f   HMD: %s\n"
                    " EyeHeight: %3.2f, IPD: %3.1fmm\n" //", Lens: %s\n"
                    " FOV %3.1fx%3.1f, Resolution: %ix%i\n"
                    "%s",
                    RadToDegree(hmdYaw), RadToDegree(hmdPitch), RadToDegree(hmdRoll),
                    RadToDegree(ThePlayer.BodyYaw.Get()),       // deliberately not GetApparentBodyYaw()
					ThePlayer.HeadPose.Translation.x, ThePlayer.HeadPose.Translation.y, ThePlayer.HeadPose.Translation.z,
                    FPS, SecondsPerFrame * 1000.0f, FrameCounter, TotalFrameCounter % 2,
                    InterAxialDistance * 1000.0f,   // convert to millimeters
                    ThePlayer.BodyPos.x, ThePlayer.BodyPos.y, ThePlayer.BodyPos.z,
                    //GetDebugNameHmdType ( TheHmdRenderInfo.HmdType ),
                    HmdDesc.ProductName,
                    ThePlayer.UserEyeHeight,
                    ovr_GetFloat(Hmd, OVR_KEY_IPD, 0) * 1000.0f,
                    //( EyeOffsetFromNoseLeft + EyeOffsetFromNoseRight ) * 1000.0f,
                    //GetDebugNameEyeCupType ( TheHmdRenderInfo.EyeCups ),  // Lens/EyeCup not exposed

                    (leftFov.GetHorizontalFovDegrees() + rightFov.GetHorizontalFovDegrees()) * 0.5f,
                    (leftFov.GetVerticalFovDegrees() + rightFov.GetVerticalFovDegrees()) * 0.5f,

                    pixelSizeWidth, pixelSizeHeight,

                    latency2Text
                    );

#if 0   // Enable if interested in texture memory usage stats
        size_t texMemInMB = pRender->GetTotalTextureMemoryUsage() / (1024 * 1024); // 1 MB
        if (texMemInMB)
        {
            char gpustat[256];
            OVR_sprintf(gpustat, sizeof(gpustat), "\nGPU Tex: %u MB", texMemInMB);
            OVR_strcat(buf, sizeof(buf), gpustat);
        }
#endif

        bounds = DrawTextBox(pRender, centerX, centerY, textHeight, buf, DrawText_Center);
    }
    break;

    case Text_Timing:
        bounds = Profiler.DrawOverlay(pRender, centerX, centerY, textHeight);
        break;

    case Text_TouchState:
        bounds = Recti(0, 0, 0, 0);
        if (HasInputState)
            bounds = RenderControllerStateHud(centerX, centerY, textHeight, InputState, 
                                              ovrControllerType_Touch);
        break;

    case Text_GamepadState:
        bounds = Recti(0, 0, 0, 0);
        if (HasInputState)
            bounds = RenderControllerStateHud(centerX, centerY, textHeight, GamepadInputState,
                                              ovrControllerType_XBox);
        break;

    case Text_Help1:
        bounds = DrawTextBox(pRender, centerX, centerY, textHeight, HelpText1, DrawText_Center);
        break;
    case Text_Help2:
        bounds = DrawTextBox(pRender, centerX, centerY, textHeight, HelpText2, DrawText_Center);
        break;

    case Text_None:
        bounds = Recti (  0, 0, 0, 0 );
        break;

    default:
        OVR_ASSERT ( !"Missing text screen" );
        break;
    }

    return bounds;
}


Recti ExperimentalWorldApp::RenderControllerStateHud(float cx, float cy, float textHeight,
                                                   const ovrInputState& is, unsigned controllerType)
{
    Recti bounds;
    
    // Button press state
    String  s = "Buttons: [";
    if (is.Buttons & ovrButton_A)           s += "A ";
    if (is.Buttons & ovrButton_B)           s += "B ";
    if (is.Buttons & ovrButton_LThumb)      s += "LThumb ";
    if (is.Buttons & ovrButton_LShoulder)   s += "LShoulder ";
    if (is.Buttons & ovrButton_X)           s += "X ";
    if (is.Buttons & ovrButton_Y)           s += "Y ";
    if (is.Buttons & ovrButton_RThumb)      s += "RThumb ";
    if (is.Buttons & ovrButton_RShoulder)   s += "RShoulder ";
    s += "] \n";

    if (controllerType & ovrControllerType_Touch)
    {
        // Touch state
        s += "Touching [";
        if (is.Touches & ovrTouch_A)         s += "A ";
        if (is.Touches & ovrTouch_B)         s += "B ";
        if (is.Touches & ovrTouch_LThumb)    s += "LThumb ";
        if (is.Touches & ovrTouch_LIndexTrigger)  s += "LIndexTrigger ";
        if (is.Touches & ovrTouch_X)         s += "X ";
        if (is.Touches & ovrTouch_Y)         s += "Y ";
        if (is.Touches & ovrTouch_RThumb)    s += "RThumb ";
        if (is.Touches & ovrTouch_RIndexTrigger)  s += "RIndexTrigger ";
        s += "] \n";
        // Do gestures on a separate line for readability
        s += "Gesture [";
        if (is.Touches & ovrTouch_LThumbUp)        s += "LThumbUp ";
        if (is.Touches & ovrTouch_RThumbUp)        s += "RThumbUp ";
        if (is.Touches & ovrTouch_LIndexPointing)  s += "LIndexPoint ";
        if (is.Touches & ovrTouch_RIndexPointing)  s += "RIndexPoint ";
        s += "] \n";
    }

    const char* deviceName     = "XBox ";
    unsigned    controllerMask = ovrControllerType_XBox;


    if (controllerType & controllerMask)
    {
        if (is.ConnectedControllerTypes & controllerMask)
        {
            // Button press state
            s += String(deviceName) + "Buttons: [";

            if (is.Buttons & ovrButton_Up)      s += "Up ";
            if (is.Buttons & ovrButton_Down)    s += "Down ";
            if (is.Buttons & ovrButton_Left)    s += "Left ";
            if (is.Buttons & ovrButton_Right)   s += "Right ";
            if (is.Buttons & ovrButton_Enter)   s += "Enter ";
            if (is.Buttons & ovrButton_Back)    s += "Back ";

            s += "] \n";
        }
        else
        {
            s += String(deviceName) + "[Not Connected]\n";
        }
    }    


    if ((controllerType & ovrControllerType_Touch) &&
        (is.ConnectedControllerTypes & ovrControllerType_Touch))
    {
        s += "Tracking: ";
        if (HandStatus[ovrHand_Left] & ovrStatus_PositionTracked)
            s += "L_Pos ";
        if (HandStatus[ovrHand_Left] & ovrStatus_OrientationTracked)
            s += "L_IMU ";

        if (HandStatus[ovrHand_Right] & ovrStatus_PositionTracked)
            s += "R_Pos ";
        if (HandStatus[ovrHand_Right] & ovrStatus_OrientationTracked)
            s += "R_IMU ";
        s += "\n";
    }


    if ((controllerType & ovrControllerType_XBox) &&
        (is.ConnectedControllerTypes & ovrControllerType_XBox))
    {
        char lbuffer[256];
        OVR_sprintf(lbuffer, sizeof(lbuffer), "Gamepad:  LT:%0.3f RT:%0.3f \n"
                                              "          LX:%0.3f,LY:%0.3f  RX:%0.3f,RY:%0.3f \n",
            is.IndexTrigger[ovrHand_Left], is.IndexTrigger[ovrHand_Right],
            is.Thumbstick[ovrHand_Left].x, is.Thumbstick[ovrHand_Left].y,
            is.Thumbstick[ovrHand_Right].x, is.Thumbstick[ovrHand_Right].y);
        s += lbuffer;
    }

    if (controllerType & ovrControllerType_Touch)
    {
        // If controllers are connected, display their axis/trigger status.
        if (is.ConnectedControllerTypes & ovrControllerType_LTouch)
        {
            char lbuffer[256];
            OVR_sprintf(lbuffer, sizeof(lbuffer), "Left Touch:  T:%0.3f M:%0.3f   X:%0.3f,Y:%0.3f\n",
                is.IndexTrigger[ovrHand_Left], is.HandTrigger[ovrHand_Left],
                is.Thumbstick[ovrHand_Left].x, is.Thumbstick[ovrHand_Left].y);
            s += lbuffer;
        }
        else
        {
            s += "Left Touch [Not Connected]\n";
        }

        if (is.ConnectedControllerTypes & ovrControllerType_RTouch)
        {
            char rbuffer[256];
            OVR_sprintf(rbuffer, sizeof(rbuffer), "Right Touch: T:%0.3f M:%0.3f   X:%0.3f,Y:%0.3f",
                is.IndexTrigger[ovrHand_Right], is.HandTrigger[ovrHand_Right],
                is.Thumbstick[ovrHand_Right].x, is.Thumbstick[ovrHand_Right].y);
            s += rbuffer;
        }
        else
        {
            s += "Right Touch [Not Connected]";
        }        
    }

    bounds = DrawTextBox(pRender, cx, cy, textHeight, s.ToCStr(), DrawText_Center);
    
    return bounds;
}


//-----------------------------------------------------------------------------
// ***** Callbacks For Menu changes

// Non-trivial callback go here.

void ExperimentalWorldApp::HmdSettingChangeFreeRTs(OptionVar*)
{
    HmdSettingsChanged = true;
    // Cause the RTs to be recreated with the new mode.
    for ( int rtNum = 0; rtNum < Rendertarget_LAST; rtNum++ )
    {
        RenderTargets[rtNum].pColorTex = NULL;
        RenderTargets[rtNum].pDepthTex = NULL;
        MsaaRenderTargets[rtNum].pColorTex = NULL;
        MsaaRenderTargets[rtNum].pDepthTex = NULL;
    }
}

void ExperimentalWorldApp::RendertargetFormatChange(OptionVar*)
{
    HmdSettingChangeFreeRTs();
}

void ExperimentalWorldApp::SrgbRequestChange(OptionVar* opt)
{
    // Reload scene with new srgb flag applied to textures
    LoadingState = LoadingState_DoLoad;

    HmdSettingChangeFreeRTs(opt);
}

void ExperimentalWorldApp::DebugHudSettingQuadPropChange(OptionVar*)
{
    switch ( DebugHudStereoPresetMode )
    {
    case DebugHudStereoPreset_Free:
        // Don't change the values.
        break;
    case DebugHudStereoPreset_Off:
        DebugHudStereoMode = ovrDebugHudStereo_Off;
        break;
    case DebugHudStereoPreset_OneMeterTwoMetersAway:
        DebugHudStereoMode = ovrDebugHudStereo_QuadWithCrosshair;
        DebugHudStereoGuideSize.x = 1.0f;
        DebugHudStereoGuideSize.y = 1.0f;
        DebugHudStereoGuidePosition.x = 0.0f;
        DebugHudStereoGuidePosition.y = 0.0f;
        DebugHudStereoGuidePosition.z = -2.0f;
        DebugHudStereoGuideYawPitchRollDeg.x = 0.0f;
        DebugHudStereoGuideYawPitchRollDeg.y = 0.0f;
        DebugHudStereoGuideYawPitchRollDeg.z = 0.0f;
        DebugHudStereoGuideColor.x = 1.0f;
        DebugHudStereoGuideColor.y = 0.5f;
        DebugHudStereoGuideColor.z = 0.1f;
        DebugHudStereoGuideColor.w = 0.8f;
        break;
    case DebugHudStereoPreset_HugeAndBright:
        DebugHudStereoMode = ovrDebugHudStereo_Quad;
        DebugHudStereoGuideSize.x = 10.0f;
        DebugHudStereoGuideSize.y = 10.0f;
        DebugHudStereoGuidePosition.x = 0.0f;
        DebugHudStereoGuidePosition.y = 0.0f;
        DebugHudStereoGuidePosition.z = -1.0f;
        DebugHudStereoGuideYawPitchRollDeg.x = 0.0f;
        DebugHudStereoGuideYawPitchRollDeg.y = 0.0f;
        DebugHudStereoGuideYawPitchRollDeg.z = 0.0f;
        DebugHudStereoGuideColor.x = 1.0f;
        DebugHudStereoGuideColor.y = 1.0f;
        DebugHudStereoGuideColor.z = 1.0f;
        DebugHudStereoGuideColor.w = 1.0f;
        break;
    case DebugHudStereoPreset_HugeAndDim:
        DebugHudStereoMode = ovrDebugHudStereo_Quad;
        DebugHudStereoGuideSize.x = 10.0f;
        DebugHudStereoGuideSize.y = 10.0f;
        DebugHudStereoGuidePosition.x = 0.0f;
        DebugHudStereoGuidePosition.y = 0.0f;
        DebugHudStereoGuidePosition.z = -1.0f;
        DebugHudStereoGuideYawPitchRollDeg.x = 0.0f;
        DebugHudStereoGuideYawPitchRollDeg.y = 0.0f;
        DebugHudStereoGuideYawPitchRollDeg.z = 0.0f;
        DebugHudStereoGuideColor.x = 0.1f;
        DebugHudStereoGuideColor.y = 0.1f;
        DebugHudStereoGuideColor.z = 0.1f;
        DebugHudStereoGuideColor.w = 1.0f;
        break;
    default: OVR_ASSERT ( false ); break;
    }

    DebugHudSettingModeChange();

    ovr_SetBool      (Hmd, OVR_DEBUG_HUD_STEREO_GUIDE_INFO_ENABLE, DebugHudStereoGuideInfoEnable);
    ovr_SetFloatArray(Hmd, OVR_DEBUG_HUD_STEREO_GUIDE_SIZE, &DebugHudStereoGuideSize[0], DebugHudStereoGuideSize.ElementCount);
    ovr_SetFloatArray(Hmd, OVR_DEBUG_HUD_STEREO_GUIDE_POSITION, &DebugHudStereoGuidePosition[0], DebugHudStereoGuidePosition.ElementCount);
    DebugHudStereoGuideYawPitchRollRad.x = DegreeToRad(DebugHudStereoGuideYawPitchRollDeg.x);
    DebugHudStereoGuideYawPitchRollRad.y = DegreeToRad(DebugHudStereoGuideYawPitchRollDeg.y);
    DebugHudStereoGuideYawPitchRollRad.z = DegreeToRad(DebugHudStereoGuideYawPitchRollDeg.z);
    ovr_SetFloatArray(Hmd, OVR_DEBUG_HUD_STEREO_GUIDE_YAWPITCHROLL, &DebugHudStereoGuideYawPitchRollRad[0], DebugHudStereoGuideYawPitchRollRad.ElementCount);
    ovr_SetFloatArray(Hmd, OVR_DEBUG_HUD_STEREO_GUIDE_COLOR, &DebugHudStereoGuideColor[0], DebugHudStereoGuideColor.ElementCount);
}

void ExperimentalWorldApp::CenterPupilDepthChange(OptionVar*)
{
    ovr_SetFloat(Hmd, "CenterPupilDepth", CenterPupilDepthMeters);
}

void ExperimentalWorldApp::DistortionClearColorChange(OptionVar*)
{
    float clearColor[2][4] = { { 0.0f, 0.0f, 0.0f, 0.0f },
                               { 0.0f, 0.5f, 1.0f, 0.0f } };
    ovr_SetFloatArray(Hmd, "DistortionClearColor",
                         clearColor[(int)DistortionClearBlue], 4);
}

void ExperimentalWorldApp::WindowSizeChange(OptionVar*)
{
    pPlatform->SetWindowSize(WindowSize.w, WindowSize.h);

    RenderParams.Resolution = WindowSize;
	if ( pRender != nullptr )
	{
		pRender->SetParams(RenderParams);
	}
}

void ExperimentalWorldApp::WindowSizeToNativeResChange(OptionVar*)
{
    pPlatform->SetWindowSize(HmdDesc.Resolution.w, HmdDesc.Resolution.h);

    RenderParams.Resolution = WindowSize;
    pRender->SetParams(RenderParams);
}

void ExperimentalWorldApp::ResetHmdPose(OptionVar* /* = 0 */)
{
    ovr_RecenterPose(Hmd);
    Menu.SetPopupMessage("Sensor Fusion Recenter Pose");
}


//-----------------------------------------------------------------------------

void ExperimentalWorldApp::ProcessDeviceNotificationQueue()
{
    // TBD: Process device plug & Unplug
}


void ExperimentalWorldApp::DisplayLastErrorMessageBox(const char* pMessage)
{
    ovrErrorInfo errorInfo;
    ovr_GetLastErrorInfo(&errorInfo);

    if(InteractiveMode)
    {
        OVR::Util::DisplayMessageBoxF("ExperimentalWorld", "%s %x -- %s", pMessage, errorInfo.Result, errorInfo.ErrorString);
    }

    OVR_DEBUG_LOG(("ExperimentalWorld: %s %x -- %s", pMessage, errorInfo.Result, errorInfo.ErrorString));
}


void ExperimentalWorldApp::GamepadStateChanged(const GamepadState& pad)
{
    ThePlayer.GamepadMove   = Vector3f(pad.LX * pad.LX * (pad.LX > 0 ? 1 : -1),
                             0,
                             pad.LY * pad.LY * (pad.LY > 0 ? -1 : 1));
    ThePlayer.GamepadRotate = Vector3f(2 * pad.RX, -2 * pad.RY, 0);

    uint32_t gamepadDeltas = (pad.Buttons ^ LastGamepadState.Buttons) & pad.Buttons;

    if (gamepadDeltas)
    {
        bool handled = Menu.OnGamepad(gamepadDeltas);
        if ( !handled )
        {
            if ( ComfortTurnMode != ComfortTurn_Off )
            {
                float howMuch = 0.0f;
                switch ( ComfortTurnMode )
                {
                case ComfortTurn_30Degrees: howMuch = DegreeToRad ( 30.0f ); break;
                case ComfortTurn_45Degrees: howMuch = DegreeToRad ( 45.0f ); break;
                default: OVR_ASSERT ( false ); break;
                }
                if ( ( gamepadDeltas & Gamepad_L1 ) != 0 )
                {
                    ThePlayer.BodyYaw += howMuch;
                }
                if ( ( gamepadDeltas & Gamepad_R1 ) != 0 )
                {
                    ThePlayer.BodyYaw -= howMuch;
                }
            }
        }
    }

    LastGamepadState = pad;
}


//-------------------------------------------------------------------------------------

OVR_PLATFORM_APP(ExperimentalWorldApp);
