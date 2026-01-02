# 700106 / 700120 Lab Book

## Final Lab - The Orb

| Category | Class Name | Role | Responsibilities |
| :--- | :--- | :--- | :--- |
| **Core System** | `Application` | Main Engine Class | Ties subsystems together (Window, Renderer, Scene); manages the main game loop, time scaling, and cleanup. |
| | `Window` | Windowing Wrapper | Wraps GLFW to create the OS window, handles resizing events, captures input, and manages the Vulkan surface connection. |
| | `ConfigLoader` | Configuration Manager | Loads data-driven settings (window size, seasons, procedural params) from files into the `AppConfig` structure. |
| **Rendering & Scene** | `Renderer` | Pipeline Conductor | Orchestrates the multi-pass rendering (Shadow -> Refraction -> Main); manages frame synchronization and command buffer recording. |
| | `Scene` | World Manager | Central hub for game entities (Terrain, Lights, Objects); handles game logic like seasons, weather, and thermodynamics. |
| | `Camera` | Observer | Maintains position/orientation vectors and calculates View/Projection matrices. |
| | `CameraController` | Input Handler | Translates keyboard/mouse input into camera movement; supports Free Roam and Orbit modes. |
| | `GraphicsPipeline` | State Definition | Configures shaders, vertex input, rasterization, depth testing, and blending. |
| | `SkyboxPass` | Environment Renderer | Renders a cubemap texture to simulate the distant background environment. |
| | `ShadowPass` | Shadow Renderer | Renders the scene depth from the light's perspective into a texture for shadow mapping. |
| | `ParticleSystem` | Effect Manager | Updates and renders particle effects using specific pipelines (additive or alpha-blended). |
| | `ParticleLibrary` | Effect Factory | Provides static helper methods to return pre-configured properties for effects like Fire, Smoke, and Rain. |
| **Geometry & Resources** | `Geometry` | Mesh Wrapper | Manages Vertex and Index buffers in GPU memory and issues draw calls. |
| | `GeometryGenerator` | Mesh Factory | Generates vertex data for primitives (Spheres, Cubes) and procedural terrain. |
| | `OBJLoader` | Asset Importer | Parses `.obj` files to load external 3D models into Geometry objects. |
| | `Texture` | Image Wrapper | Loads 2D images, creates Vulkan image views/samplers, and manages descriptor sets. |
| | `Cubemap` | Texture Wrapper | Loads 6 individual images to create a cube-compatible Vulkan image view. |
| **Vulkan Abstraction** | `VulkanContext` | Root State | Initializes the Vulkan Instance, Debug Messenger, and Window Surface. |
| | `VulkanDevice` | GPU Interface | Selects the Physical Device, creates the Logical Device, and retrieves Graphics/Present queues. |
| | `VulkanSwapChain` | Presentation Manager | Creates the Swapchain and Image Views; handles surface formats and resolution. |
| | `VulkanRenderPass` | Pass Definition | Configures attachments (Color, Depth), subpasses, and dependencies. |
| | `VulkanShader` | Module Loader | Reads SPIR-V binary code and creates `VkShaderModule` objects. |
| | `VulkanCommandBuffer` | Command Manager | Manages the Command Pool and allocates Command Buffers for recording. |
| | `VulkanBuffer` | Memory Manager | Allocates GPU memory for Vertex, Index, or Uniform buffers. |
| | `VulkanDescriptorSet` | Resource Binder | Manages layouts, pools, and sets to allow shaders to access buffers and textures. |
| | `VulkanSyncObjects` | Synchronization | Manages Semaphores and Fences to coordinate CPU-GPU and GPU-GPU execution. |
| | `VulkanUtils` | Helper Library | Provides utilities for common tasks like image creation and layout transitions. |

##
```mermaid
classDiagram
    namespace Core {
        class Application {
            -unique_ptr~Window~ window
            -unique_ptr~VulkanContext~ vulkanContext
            -unique_ptr~VulkanDevice~ vulkanDevice
            -unique_ptr~VulkanSwapChain~ vulkanSwapChain
            -unique_ptr~Renderer~ renderer
            -unique_ptr~Scene~ scene
            -unique_ptr~CameraController~ cameraController
            -AppConfig config
            +Run()
            -InitVulkan()
            -MainLoop()
        }

        class Window {
            +GetGLFWWindow()
            +PollEvents()
            +ShouldClose()
        }

        class ConfigLoader {
            +Load(filepath) AppConfig
        }

        class AppConfig {
            +int windowWidth
            +int windowHeight
            +SeasonConfig seasons
            +OrbitConfig sunOrbit
        }
    }

    namespace Logic {
        class CameraController {
            -Camera* activeCamera
            +SwitchCamera(type)
            +Update(deltaTime)
            +ProcessInput()
        }

        class Camera {
            +GetViewMatrix()
            +GetProjectionMatrix()
            +GetPosition()
        }
    }

    Application *-- Window : owns
    Application *-- CameraController : owns
    Application ..> ConfigLoader : uses
    Application *-- AppConfig : contains
    CameraController o-- Camera : manages
    Application *-- Renderer : owns
    Application *-- Scene : owns
```
##
```mermaid
classDiagram
    namespace Rendering {
        class Renderer {
            -VulkanDevice* device
            -VulkanSwapChain* swapChain
            -unique_ptr~VulkanRenderPass~ renderPass
            -unique_ptr~GraphicsPipeline~ graphicsPipeline
            -unique_ptr~VulkanCommandBuffer~ commandBuffer
            -unique_ptr~VulkanSyncObjects~ syncObjects
            -unique_ptr~ShadowPass~ shadowPass
            -unique_ptr~SkyboxPass~ skyboxPass
            +DrawFrame()
            +Initialize()
        }

        class Scene {
            -vector~unique_ptr~SceneObject~~ objects
            -vector~unique_ptr~ParticleSystem~~ particleSystems
            +Update(deltaTime)
            +GenerateProceduralObjects()
            +UpdateThermodynamics()
        }

        class SceneObject {
            +string name
            +shared_ptr~Geometry~ geometry
            +Transform transform
            +ObjectState state
        }

        class ShadowPass {
            +RenderShadowMap()
        }

        class SkyboxPass {
            +Draw()
        }

        class ParticleSystem {
            +Update(deltaTime)
            +Draw(cmd)
        }

        class GraphicsPipeline

    }

    namespace VulkanAbstraction {
        class VulkanContext {
            +CreateInstance()
        }
        class VulkanDevice {
            +PickPhysicalDevice()
            +CreateLogicalDevice()
        }
        class VulkanSwapChain {
            +Create()
            +CreateImageViews()
        }
        class VulkanRenderPass
        class VulkanCommandBuffer
        class VulkanSyncObjects
        class VulkanDescriptorSet
        class VulkanBuffer
    }

    namespace Assets_Geometry {
        class Geometry {
            +Bind(cmd)
            +Draw(cmd)
        }
        class Texture {
            +LoadFromFile()
        }
        class OBJLoader {
            +Load(path) Geometry
        }
        class GeometryGenerator {
            +CreateSphere()
            +CreateTerrain()
        }
    }

    %% Relationships
    Renderer *-- VulkanRenderPass : owns
    Renderer *-- GraphicsPipeline : owns
    Renderer *-- VulkanCommandBuffer : owns
    Renderer *-- VulkanSyncObjects : owns
    Renderer *-- VulkanDescriptorSet : owns
    Renderer *-- ShadowPass : owns
    Renderer *-- SkyboxPass : owns

    Renderer o-- VulkanDevice : uses
    Renderer o-- VulkanSwapChain : uses
    
    Scene *-- SceneObject : owns
    Scene *-- ParticleSystem : owns
    SceneObject o-- Geometry : shares
    Scene ..> OBJLoader : uses
    Scene ..> GeometryGenerator : uses
    
    GraphicsPipeline ..> VulkanShader : uses
    Geometry ..> VulkanBuffer : uses
```
##
```mermaid
sequenceDiagram
    autonumber
    participant Main
    participant App as Application
    participant Win as Window
    participant VkCtx as VulkanContext
    participant VkDev as VulkanDevice
    participant Rend as Renderer
    participant Scene as Scene

    Main->>App: Application()
    activate App
    
    App->>Win: Create Window
    activate Win
    Win-->>App: GLFW Handle
    deactivate Win

    App->>App: InitVulkan()
    activate App
    App->>VkCtx: CreateInstance()
    App->>VkCtx: CreateSurface(window)
    App->>VkDev: PickPhysicalDevice()
    App->>VkDev: CreateLogicalDevice()
    deactivate App

    App->>Rend: new Renderer(device, swapchain)
    activate Rend
    Rend->>Rend: Initialize()
    note right of Rend: Creates RenderPass, Pipelines, CommandBuffers
    deactivate Rend

    App->>App: SetupScene()
    activate App
    App->>Scene: new Scene(device)
    App->>Scene: Initialize()
    App->>Scene: AddObject("Terrain", ...)
    App->>Scene: AddLight("Sun", ...)
    deactivate App

    Main->>App: Run()
    deactivate App
```
##
```mermaid
sequenceDiagram
    autonumber
    participant App as Application
    participant Scene as Scene
    participant Cam as CameraController
    participant Rend as Renderer
    participant Swap as VulkanSwapChain
    participant GPU as VulkanDevice/Queue

    loop MainLoop
        App->>App: Calculate deltaTime
        
        %% Input & Logic Phase
        App->>Win: PollEvents()
        App->>Cam: Update(deltaTime)
        App->>Scene: Update(deltaTime)
        activate Scene
        Scene->>Scene: UpdateThermodynamics()
        Scene->>Scene: UpdateParticles()
        deactivate Scene

        %% Rendering Phase
        App->>Rend: DrawFrame(Scene, frameIndex)
        activate Rend
        
        Rend->>GPU: WaitForFences(inFlightFence)
        Rend->>Swap: AcquireNextImage()
        Swap-->>Rend: imageIndex

        Rend->>Rend: UpdateUniformBuffer(ubo)
        
        Rend->>Rend: RecordCommandBuffer(cmd, imageIndex)
        activate Rend
        note right of Rend: Binds Pipeline, DescSets, Buffers
        Rend->>Scene: Draw(cmd)
        deactivate Rend

        Rend->>GPU: Submit(graphicsQueue, cmd)
        Rend->>Swap: QueuePresent(presentQueue, imageIndex)
        
        deactivate Rend
    end
```
##
```mermaid
sequenceDiagram
    participant Scene
    participant PS as ParticleSystem
    participant Rend as Renderer
    participant Cmd as VkCommandBuffer

    %% Update Step (CPU Simulation)
    rect rgb(30, 30, 30)
    note right of Scene: Logic Update
    Scene->>PS: Update(deltaTime)
    activate PS
    PS->>PS: Emit new particles
    PS->>PS: Update positions/life
    PS->>PS: UpdateInstanceBuffer()
    note right of PS: Maps memory & copies data to GPU
    deactivate PS
    end

    %% Draw Step (GPU Recording)
    rect rgb(50, 20, 20)
    note right of Scene: Render Pass
    Rend->>Scene: RenderScene(cmd)
    activate Scene
    Scene->>PS: Draw(cmd, globalDescSet)
    activate PS
    PS->>Cmd: vkCmdBindPipeline(particlePipeline)
    PS->>Cmd: vkCmdBindDescriptorSets(texture)
    PS->>Cmd: vkCmdBindVertexBuffers(quad + instance)
    PS->>Cmd: vkCmdDrawIndexed(6, particleCount, ...)
    deactivate PS
    deactivate Scene
    end
```
##
### Q1. 

**Question:**


**Solution:**


**Sample output:**


**Reflection:**


**Questions:**
