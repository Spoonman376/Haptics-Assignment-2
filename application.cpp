//==============================================================================
/*
    CPSC 599.86 / 601.86 - Computer Haptics
    Winter 2017, University of Calgary

    You may use this program as a boilerplate for starting assignment #2.

    Additional files to support a custom subclass of cMesh that constructs
    a rough triangle mesh from an implicit surface function are provided.
    This class uses a public domain implementation of the marching cubes
    algorithm to extract a polygonal mesh from the surface function for
    visual rendering.

    You will likely write most of your code in the ImplicitMesh class for
    this homework assignment.

    \author    Your Name
*/
//==============================================================================

//------------------------------------------------------------------------------
#include "chai3d.h"
#include "ImplicitMesh.h"
//------------------------------------------------------------------------------
#include <GLFW/glfw3.h>
//------------------------------------------------------------------------------
using namespace chai3d;
using namespace std;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GENERAL SETTINGS
//------------------------------------------------------------------------------

// stereo Mode
/*
    C_STEREO_DISABLED:            Stereo is disabled 
    C_STEREO_ACTIVE:              Active stereo for OpenGL NVDIA QUADRO cards
    C_STEREO_PASSIVE_LEFT_RIGHT:  Passive stereo where L/R images are rendered next to each other
    C_STEREO_PASSIVE_TOP_BOTTOM:  Passive stereo where L/R images are rendered above each other
*/
cStereoMode stereoMode = C_STEREO_DISABLED;

// fullscreen mode
bool fullscreen = false;

// mirrored display
bool mirroredDisplay = false;


//------------------------------------------------------------------------------
// DECLARED VARIABLES
//------------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera to render the world in the window display
cCamera* camera;

// a light source to illuminate the objects in the world
cSpotLight *light;

// a haptic device handler
cHapticDeviceHandler* handler;

// a pointer to the current haptic device
cGenericHapticDevicePtr hapticDevice;

// a label to display the rates [Hz] at which the simulation is running
cLabel* labelRates;

// a virtual tool representing the haptic device in the scene
cToolCursor* tool;

ImplicitMesh *sphere;
ImplicitMesh *heart;
ImplicitMesh *cube;
ImplicitMesh *shape;
ImplicitMesh *plane;

// flag to indicate if the haptic simulation currently running
bool simulationRunning = false;

// flag to indicate if the haptic simulation has terminated
bool simulationFinished = false;

// a frequency counter to measure the simulation graphic rate
cFrequencyCounter freqCounterGraphics;

// a frequency counter to measure the simulation haptic rate
cFrequencyCounter freqCounterHaptics;

// haptic thread
cThread* hapticsThread;

// a handle to window display context
GLFWwindow* window = NULL;

// current width of window
int width  = 0;

// current height of window
int height = 0;

// swap interval for the display context (vertical synchronization)
int swapInterval = 1;


//------------------------------------------------------------------------------
// DECLARED FUNCTIONS
//------------------------------------------------------------------------------

// callback when the window display is resized
void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height);

// callback when an error GLFW occurs
void errorCallback(int error, const char* a_description);

// callback when a key is pressed
void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods);

// this function renders the scene
void updateGraphics(void);

// this function contains the main haptics simulation loop
void updateHaptics(void);

// this function closes the application
void close(void);


// [CPSC.86] sample implicit function for a sphere
double implicitSphere(double x, double y, double z)
{
  return x * x + y * y + z * z - 1;
}

chai3d::cVector3d sphereGradient(double x, double y, double z)
{
  return chai3d::cVector3d(2 * x, 2 * y, 2 * z);
}     

double implicitPlane(double x, double y, double z)
{
  return x;
}

chai3d::cVector3d planeGradient(double x, double y, double z)
{
  return chai3d::cVector3d(1,0,0);
}


double implicitHeart(double x, double y, double z)
{
  return pow(2 * x * x + y * y + z * z - 1, 3) - (0.1 * x * x + y *  y) * z * z * z;
}

chai3d::cVector3d heartGradient(double x, double y, double z)
{
  return chai3d::cVector3d(12 * x * pow(2 * x * x + y * y + z * z - 1, 2) - 0.2 * x * z * z * z,
                           6 * y * pow(2 * x * x + y * y + z * z - 1, 2) - 2 * y * z * z * z,
                           6 * z * pow(2 * x * x + y * y + z * z - 1, 2) - 3 * z * z * (0.1 * x * x + y * y));
}

double implicitWhiffleCube(double x, double y, double z)
{
  return pow(pow(x, 8) + pow(y, 8) + pow(z, 8), 8) + pow(x * x + y * y + z * z - 0.44, -8) - 1;
}

chai3d::cVector3d whiffleCubeGradient(double x, double y, double z)
{
  return chai3d::cVector3d(64 * pow(x, 7) * pow(pow(x, 8) + pow(y, 8) + pow(z, 8), 7) - 16 * x * pow(x * x + y * y + z * z - 0.44, -9),
                           64 * pow(y, 7) * pow(pow(x, 8) + pow(y, 8) + pow(z, 8), 7) - 16 * y * pow(x * x + y * y + z * z - 0.44, -9),
                           64 * pow(z, 7) * pow(pow(x, 8) + pow(y, 8) + pow(z, 8), 7) - 16 * z * pow(x * x + y * y + z * z - 0.44, -9));
}


double implicitShape(double x, double y, double z)
{
  return 4 * z * z * z * z + 0.64 * (x * x + y * y - 4 * z * z);
}

chai3d::cVector3d shapeGradient(double x, double y, double z)
{
  return chai3d::cVector3d(2 * 0.64 * x, 2 * 0.64 * y, 16 * z * z * z - 8 * 0.64 * z);
}


void setFrictions(double s, double k)
{
  sphere->setFriction(s, k);
  heart->setFriction(s, k);
  cube->setFriction(s, k);
  shape->setFriction(s, k);
  plane->setFriction(s, k);
}

//==============================================================================
/*
    Here is the main application entry point.  This program follows the same
    structure as the CHAI3D example programs.

    The scene graph in this template program contains one primary object of
    type ImplicitMesh, constructed using a sample function for an implicit
    sphere defined above.  The implementation of the force rendering
    algorithm must be completed in the ImplicitMesh class before the object
    can be touched.
*/
//==============================================================================

int main(int argc, char* argv[])
{
    //--------------------------------------------------------------------------
    // INITIALIZATION
    //--------------------------------------------------------------------------

    cout << endl;
    cout << "-----------------------------------" << endl;
    cout << "CPSC 599.86/601.86 - Computer Haptics" << endl;
    cout << "Assignment #2: Implicit Surfaces" << endl;
    cout << "Winter 2017, University of Calgary" << endl;
    cout << "Copyright 2003-2017" << endl;
    cout << "-----------------------------------" << endl << endl << endl;
    cout << "Keyboard Options:" << endl << endl;
    cout << "[1-5] - Switch bewteen scenes" << endl;
    cout << "[f] - Enable/Disable full screen mode" << endl;
    cout << "[m] - Enable/Disable vertical mirroring" << endl;
    cout << "[q] - Exit application" << endl;
    cout << endl << endl;


    //--------------------------------------------------------------------------
    // OPENGL - WINDOW DISPLAY
    //--------------------------------------------------------------------------

    // initialize GLFW library
    if (!glfwInit())
    {
        cout << "failed initialization" << endl;
        cSleepMs(1000);
        return 1;
    }

    // set error callback
    glfwSetErrorCallback(errorCallback);

    // compute desired size of window
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int w = 0.8 * mode->height;
    int h = 0.5 * mode->height;
    int x = 0.5 * (mode->width - w);
    int y = 0.5 * (mode->height - h);

    // set OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // set active stereo mode
    if (stereoMode == C_STEREO_ACTIVE)
    {
        glfwWindowHint(GLFW_STEREO, GL_TRUE);
    }
    else
    {
        glfwWindowHint(GLFW_STEREO, GL_FALSE);
    }

    // create display context
    window = glfwCreateWindow(w, h, "CHAI3D", NULL, NULL);
    if (!window)
    {
        cout << "failed to create window" << endl;
        cSleepMs(1000);
        glfwTerminate();
        return 1;
    }

    // get width and height of window
    glfwGetWindowSize(window, &width, &height);

    // set position of window
    glfwSetWindowPos(window, x, y);

    // set key callback
    glfwSetKeyCallback(window, keyCallback);

    // set resize callback
    glfwSetWindowSizeCallback(window, windowSizeCallback);

    // set current display context
    glfwMakeContextCurrent(window);

    // sets the swap interval for the current display context
    glfwSwapInterval(swapInterval);

#ifdef GLEW_VERSION
    // initialize GLEW library
    if (glewInit() != GLEW_OK)
    {
        cout << "failed to initialize GLEW library" << endl;
        glfwTerminate();
        return 1;
    }
#endif


    //--------------------------------------------------------------------------
    // WORLD - CAMERA - LIGHTING
    //--------------------------------------------------------------------------

    // create a new world.
    world = new cWorld();

    // set the background color of the environment
    world->m_backgroundColor.setWhite();

    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);

    // position and orient the camera
    camera->set( cVector3d (4.0, 0.0, 0.0),    // camera position (eye)
                 cVector3d (0.0, 0.0, 0.0),    // look at position (target)
                 cVector3d (0.0, 0.0, 1.0));   // direction of the (up) vector

    // set the near and far clipping planes of the camera
    camera->setClippingPlanes(0.01, 10.0);

    // set stereo mode
    camera->setStereoMode(stereoMode);

    // set stereo eye separation and focal length (applies only if stereo is enabled)
    camera->setStereoEyeSeparation(0.03);
    camera->setStereoFocalLength(4.0);

    // set vertical mirrored display mode
    camera->setMirrorVertical(mirroredDisplay);

    // create a spot light source
    light = new cSpotLight(world);

    // insert light source inside world
    camera->addChild(light);

    // enable light source
    light->setEnabled(true);

    // position the light source
    light->setLocalPos(0.0, 1.0, 2.0);

    // define the direction of the light beam
    light->setDir(-2.0, -0.5, -1.0);

    // enable this light source to generate shadows
    light->setShadowMapEnabled(true);

    // set the resolution of the shadow map
    light->m_shadowMap->setQualityMedium();

    // set light cone half angle
    light->setCutOffAngleDeg(30);


    //--------------------------------------------------------------------------
    // HAPTIC DEVICE
    //--------------------------------------------------------------------------

    // create a haptic device handler
    handler = new cHapticDeviceHandler();

    // get a handle to the first haptic device
    handler->getDevice(hapticDevice, 0);

    // retrieve information about the current haptic device
    cHapticDeviceInfo info = hapticDevice->getSpecifications();

    // if the device has a gripper, enable the gripper to simulate a user switch
    hapticDevice->setEnableGripperUserSwitch(true);

    // create a tool (cursor) and insert into the world
    tool = new cToolCursor(world);
    world->addChild(tool);

    // connect the haptic device to the tool
    tool->setHapticDevice(hapticDevice);

    // map the physical workspace of the haptic device to a larger virtual workspace.
    tool->setWorkspaceRadius(2.0);

    // define a radius for the virtual tool (sphere)
    tool->setRadius(0.05);

    // hide the device sphere. only show proxy.
    tool->setShowContactPoints(true, false);

    // start the haptic tool
    tool->start();


    //--------------------------------------------------------------------------
    // CREATE OBJECTS
    //--------------------------------------------------------------------------

    // read the scale factor between the physical workspace of the haptic
    // device and the virtual workspace defined for the tool
    double workspaceScaleFactor = tool->getWorkspaceScaleFactor();

    // stiffness properties
    double maxStiffness	= info.m_maxLinearStiffness / workspaceScaleFactor;


    /////////////////////////////////////////////////////////////////////////
    // [CPSC.86] IMPLICIT SURFACE OBJECT(S)
    /////////////////////////////////////////////////////////////////////////

    // create the object representing the implicit surface
    sphere = new ImplicitMesh();
    heart = new ImplicitMesh();
    cube = new ImplicitMesh();
    shape = new ImplicitMesh();
    plane = new ImplicitMesh();

    // generate a mesh for the implicit surface (inside a bounding box with
    // range -1.25 to 1.25, and a resolution of 0.05 units)
    sphere->createFromFunction(implicitSphere, sphereGradient,
                               cVector3d(-1.25, -1.25, -1.25),
                               cVector3d(1.25, 1.25, 1.25), 0.025);

    heart->createFromFunction(implicitHeart, heartGradient,
                              cVector3d(-1.25, -1.25, -1.25),
                              cVector3d(1.25, 1.25, 1.25), 0.025);

    cube->createFromFunction(implicitWhiffleCube, whiffleCubeGradient,
                             cVector3d(-1.25, -1.25, -1.25),
                             cVector3d(1.25, 1.25, 1.25), 0.025);

    shape->createFromFunction(implicitShape, shapeGradient,
                              cVector3d(-1.25, -1.25, -1.25),
                              cVector3d(1.25, 1.25, 1.25), 0.025);


    plane->createFromFunction(implicitPlane, planeGradient,
                              cVector3d(-1.25, -1.25, -1.25),
                              cVector3d(1.25, 1.25, 1.25), 0.025);

    // the surface effect renders a spring force between the device and proxy
    // points with the given stiffness in the material
    sphere->addEffect(new cEffectSurface(sphere));
    sphere->m_material->setStiffness(0.75 * maxStiffness);

    heart->addEffect(new cEffectSurface(heart));
    heart->m_material->setStiffness(0.75 * maxStiffness);

    cube->addEffect(new cEffectSurface(cube));
    cube->m_material->setStiffness(0.75 * maxStiffness);

    shape->addEffect(new cEffectSurface(shape));
    shape->m_material->setStiffness(0.75 * maxStiffness);

    plane->addEffect(new cEffectSurface(plane));
    plane->m_material->setStiffness(0.75 * maxStiffness);

    // give the surface a nice red colour
    sphere->m_material->setRedDark();
    heart->m_material->setRedDark();
    cube->m_material->setRedDark();
    shape->m_material->setRedDark();
    plane->m_material->setRedDark();

    // add some friction to the object's material
    setFrictions(0, 0);

    world->addChild(sphere);


    //--------------------------------------------------------------------------
    // WIDGETS
    //--------------------------------------------------------------------------

    // create a font
    cFontPtr font = NEW_CFONTCALIBRI20();
    
    // create a label to display the haptic and graphic rates of the simulation
    labelRates = new cLabel(font);
    labelRates->m_fontColor.setBlack();
    camera->m_frontLayer->addChild(labelRates);

    // create a background
    cBackground* background = new cBackground();
    camera->m_backLayer->addChild(background);

    // set background properties
    background->setCornerColors(cColorf(1.0, 1.0, 1.0),
                                cColorf(1.0, 1.0, 1.0),
                                cColorf(0.8, 0.8, 0.8),
                                cColorf(0.8, 0.8, 0.8));

    //--------------------------------------------------------------------------
    // START SIMULATION
    //--------------------------------------------------------------------------

    // create a thread which starts the main haptics rendering loop
    hapticsThread = new cThread();
    hapticsThread->start(updateHaptics, CTHREAD_PRIORITY_HAPTICS);

    // setup callback when application exits
    atexit(close);


    //--------------------------------------------------------------------------
    // MAIN GRAPHIC LOOP
    //--------------------------------------------------------------------------

    // call window size callback at initialization
    windowSizeCallback(window, width, height);

    // main graphic loop
    while (!glfwWindowShouldClose(window))
    {
        // get width and height of window
        glfwGetWindowSize(window, &width, &height);

        // render graphics
        updateGraphics();

        // swap buffers
        glfwSwapBuffers(window);

        // process events
        glfwPollEvents();

        // signal frequency counter
        freqCounterGraphics.signal(1);
    }

    // close window
    glfwDestroyWindow(window);

    // terminate GLFW library
    glfwTerminate();

    // exit
    return 0;
}

//------------------------------------------------------------------------------

void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height)
{
    // update window size
    width  = a_width;
    height = a_height;
}

//------------------------------------------------------------------------------

void errorCallback(int a_error, const char* a_description)
{
    cout << "Error: " << a_description << endl;
}

//------------------------------------------------------------------------------

void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods)
{
    // filter calls that only include a key press
    if (a_action != GLFW_PRESS)
    {
        return;
    }

    // option - exit
    else if ((a_key == GLFW_KEY_ESCAPE) || (a_key == GLFW_KEY_Q))
    {
        glfwSetWindowShouldClose(a_window, GLFW_TRUE);
    }

    // option - toggle fullscreen
    else if (a_key == GLFW_KEY_F)
    {
        // toggle state variable
        fullscreen = !fullscreen;

        // get handle to monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        // get information about monitor
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // set fullscreen or window mode
        if (fullscreen)
        {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
        else
        {
            int w = 0.8 * mode->height;
            int h = 0.5 * mode->height;
            int x = 0.5 * (mode->width - w);
            int y = 0.5 * (mode->height - h);
            glfwSetWindowMonitor(window, NULL, x, y, w, h, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
    }

    // option - toggle vertical mirroring
    else if (a_key == GLFW_KEY_M)
    {
        mirroredDisplay = !mirroredDisplay;
        camera->setMirrorVertical(mirroredDisplay);
    }

    else  if (a_key == GLFW_KEY_1)
    {
      world->clearAllChildren();
      world->addChild(tool);
      world->addChild(sphere);
    }

    else  if (a_key == GLFW_KEY_2)
    {
      world->clearAllChildren();
      world->addChild(tool);
      world->addChild(heart);
    }
    else  if (a_key == GLFW_KEY_3)
    {
      world->clearAllChildren();
      world->addChild(tool);
      world->addChild(cube);
    }
    else  if (a_key == GLFW_KEY_4)
    {
      world->clearAllChildren();
      world->addChild(tool);
      world->addChild(shape);
    }
    else  if (a_key == GLFW_KEY_5)
    {
      world->clearAllChildren();
      world->addChild(tool);
      world->addChild(plane);
    }
    else if (a_key == GLFW_KEY_A)
    {
      setFrictions(0, 0);
    }
    else if (a_key == GLFW_KEY_S)
    {
      setFrictions(0.3, 0.2);
    }
    else if (a_key == GLFW_KEY_D)
    {
      setFrictions(0.7, 0.5);
    }
}

//------------------------------------------------------------------------------

void close(void)
{
    // stop the simulation
    simulationRunning = false;

    // wait for graphics and haptics loops to terminate
    while (!simulationFinished) { cSleepMs(100); }

    // close haptic device
    tool->stop();

    // delete resources
    delete hapticsThread;
    delete world;
    delete handler;
}

//------------------------------------------------------------------------------

void updateGraphics(void)
{
    /////////////////////////////////////////////////////////////////////
    // UPDATE WIDGETS
    /////////////////////////////////////////////////////////////////////


    // update haptic and graphic rate data
    labelRates->setText(cStr(freqCounterGraphics.getFrequency(), 0) + " Hz / " +
        cStr(freqCounterHaptics.getFrequency(), 0) + " Hz");

    // update position of label
    labelRates->setLocalPos((int)(0.5 * (width - labelRates->getWidth())), 15);


    /////////////////////////////////////////////////////////////////////
    // RENDER SCENE
    /////////////////////////////////////////////////////////////////////

    // update shadow maps (if any)
    world->updateShadowMaps(false, mirroredDisplay);

    // render world
    camera->renderView(width, height);

    // wait until all GL commands are completed
    glFinish();

    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) cout << "Error:  %s\n" << gluErrorString(err);
}

//------------------------------------------------------------------------------

void updateHaptics(void)
{
    // simulation in now running
    simulationRunning  = true;
    simulationFinished = false;

    // main haptic simulation loop
    while(simulationRunning)
    {
        /////////////////////////////////////////////////////////////////////
        // HAPTIC FORCE COMPUTATION
        /////////////////////////////////////////////////////////////////////

        // compute global reference frames for each object
        world->computeGlobalPositions(true);

        // update position and orientation of tool
        tool->updateFromDevice();

        // compute interaction forces
        tool->computeInteractionForces();

        // send forces to haptic device
        tool->applyToDevice();

        // signal frequency counter
        freqCounterHaptics.signal(1);
    }
    
    // exit haptics thread
    simulationFinished = true;
}

//------------------------------------------------------------------------------
