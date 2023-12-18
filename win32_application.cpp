#ifdef WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wingdi.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include "d3dx12.h" // Helper Structures and Functions

#include <string>
#include <wrl.h>
using Microsoft::WRL::ComPtr;
#include <shellapi.h>

#endif // WINDOWS

#include "log.h"
#include "types.h"
#include "win32_application.h"

#include "log.cpp"

void init_dx_sample(dx_sample *sample, UINT width, UINT height) {
	sample->m_width = width;
	sample->m_height = height;
	sample->m_aspect_ratio = (float)width / (float)height;
}

void dx12_get_hardware_adapter(_In_ IDXGIFactory1* p_factory, _Outptr_result_maybenull_ IDXGIAdapter1** pp_adapter, bool request_high_performance_adapter = false);

_Use_decl_annotations_
void dx12_get_hardware_adapter(IDXGIFactory1* p_factory, IDXGIAdapter1** pp_adapter, bool request_high_performance_adapter) {
    *pp_adapter = nullptr;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(p_factory->QueryInterface(IID_PPV_ARGS(&factory6)))) {
        for (UINT adapter_index = 0;
             SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapter_index,
                request_high_performance_adapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter)));
             ++adapter_index) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
                break;
            }
        }
    }

    if(adapter.Get() == nullptr) {
        for (UINT adapter_index = 0; SUCCEEDED(p_factory->EnumAdapters1(adapter_index, &adapter)); ++adapter_index) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
                break;
            }
        }
    }
    
    *pp_adapter = adapter.Detach();
}

void dx_load_pipeline(dx_hello_triangle *input, HWND window_handle) {
	// Enable the D3D12 debug layer
	UINT dxgiFactoryFlags = 0;
#ifdef DEBUG
	ComPtr<ID3D12Debug> debug_controller;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
        debug_controller->EnableDebugLayer();

        // Enable additional debug layers.
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif // DEBUG

    // Create the factory
    ComPtr<IDXGIFactory4> factory;
    {
	    HRESULT result = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
	    if (FAILED(result)) {
	    	output("load_pipeline(): CreateDXGIFactory1() failed");
	    }
    }	

	// Create the device
	{
	    if (input->sample.m_use_warp_device) {
	    	ComPtr<IDXGIAdapter> warp_adapter;
	    	if (FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp_adapter)))) {
	    		output("load_pipeline(): EnumWarpAdapter() failed");
	    	}

	    	HRESULT result = D3D12CreateDevice(warp_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&input->m_device));
	    	if (FAILED(result)) {
	    		output("load_pipeline(): D3D12CreateDevice() warp adapter failed");
	    	}

	    } else {
	    	ComPtr<IDXGIAdapter1> hardware_adapter;
	    	dx12_get_hardware_adapter(factory.Get(), &hardware_adapter);

	    	HRESULT result = D3D12CreateDevice(hardware_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&input->m_device));
	    	if (FAILED(result)) {
	    		output("load_pipeline(): D3D12CreateDevice() hardware adapter failed");
	    	}
	    }
	}

    // Describe and create the command queue
    {
	    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	    HRESULT result = input->m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&input->m_command_queue));
	}

	// Describe and create the swap chain
	{
		DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	    swap_chain_desc.BufferCount = input->frame_count;
	    swap_chain_desc.Width = input->sample.m_width;
	    swap_chain_desc.Height = input->sample.m_height;
	    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	    swap_chain_desc.SampleDesc.Count = 1;
    
    	ComPtr<IDXGISwapChain1> swap_chain;
	    HRESULT result = factory->CreateSwapChainForHwnd(
	        input->m_command_queue.Get(), // Swap chain needs the queue so that it can force a flush on it.
            window_handle,
	        &swap_chain_desc,
            nullptr,
            nullptr,
	        &swap_chain);
	    if (FAILED(result)) {
	    	output("load_pipeline(): CreateSwapChain() failed");
	    }

        // This sample does not support fullscreen transitions
   		result = factory->MakeWindowAssociation(window_handle, DXGI_MWA_NO_ALT_ENTER);
   		if (FAILED(result)) {
   			output("load_pipeline(): MakeWindowAssociation() failed");
   		}

        result = swap_chain.As(&input->m_swap_chain);
        if (FAILED(result)) {
            output("load_pipeline(): swap_chain.As() failed");
        }

        input->m_frame_index = input->m_swap_chain->GetCurrentBackBufferIndex();
   	}

   	// Create descriptor heaps
   	{
   		// Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
        rtv_heap_desc.NumDescriptors = input->frame_count;
        rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        HRESULT result = input->m_device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&input->m_rtv_heap));
        if (FAILED(result)) {
        	output("load_pipeline(): CreateDescriptorHeap() failed");
        }

        input->m_rtv_descriptor_size = input->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
   	}

   	// Create frame resources
   	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(input->m_rtv_heap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < input->frame_count; n++) {
            HRESULT result = input->m_swap_chain->GetBuffer(n, IID_PPV_ARGS(&input->m_render_targets[n]));
            if (FAILED(result)) {
            	output("load_pipeline(): GetBuffer() failed");
            }
            input->m_device->CreateRenderTargetView(input->m_render_targets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, input->m_rtv_descriptor_size);

            result = input->m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&input->m_command_allocators[n]));
            if (FAILED(result)) {
                output("load_pipeline(): CreateCommandAllocator() failed");
            }
        }
   	}
}

// Wait for pending GPU work to complete.
void dx12_wait_for_gpu(dx_hello_triangle *input) {
    // Schedule a Signal command in the queue.
    HRESULT result = input->m_command_queue->Signal(input->m_fence.Get(), input->m_fence_values[input->m_frame_index]);
    if (FAILED(result)) output("dx12_wait_for_gpu(): Signal() failed");

    // Wait until the fence has been processed.
    result = input->m_fence->SetEventOnCompletion(input->m_fence_values[input->m_frame_index], input->m_fence_event);
    if (FAILED(result)) output("dx12_wait_for_gpu(): SetEventOnCompletion() failed");
    WaitForSingleObjectEx(input->m_fence_event, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    input->m_fence_values[input->m_frame_index]++;
}

void dx12_move_to_next_frame(dx_hello_triangle *input) {
    // Schedule a Signal command in the queue.
    const UINT64 current_fence_value = input->m_fence_values[input->m_frame_index];
    HRESULT result = input->m_command_queue->Signal(input->m_fence.Get(), current_fence_value);
    if (FAILED(result)) output("dx12_move_to_next_frame(): Signal() failed");

    // Update the frame index.
    input->m_frame_index = input->m_swap_chain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (input->m_fence->GetCompletedValue() < input->m_fence_values[input->m_frame_index])
    {
        HRESULT result = input->m_fence->SetEventOnCompletion(input->m_fence_values[input->m_frame_index], input->m_fence_event);
        if (FAILED(result)) output("dx12_move_to_next_frame(): SetEventOnCompletion() failed");
        WaitForSingleObjectEx(input->m_fence_event, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    input->m_fence_values[input->m_frame_index] = current_fence_value + 1;
}

void dx_load_assets(dx_hello_triangle *input) {
	// Create an empty root signature.
    {
        CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
        root_signature_desc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        HRESULT result = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        if (FAILED(result)) output("load_assets(): D3D12SerializeRootSignature() failed");
        result = input->m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&input->m_root_signature));
        if (FAILED(result)) output("load_assets(): CreateRootSignature() failed");
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> vertex_shader;
        ComPtr<ID3DBlob> pixel_shader;

#ifdef DEBUG
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif
        //const char *file = (const char *)read_file_terminated("../shaders.hlsl").memory;
        LPCWSTR file = L"../shaders.hlsl";

        HRESULT result;
        result = D3DCompileFromFile(file, nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertex_shader, nullptr);
        if (FAILED(result)) output("load_assets(): D3DCompileFromFile() failed");
        result = D3DCompileFromFile(file, nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixel_shader, nullptr);
        if (FAILED(result)) output("load_assets(): D3DCompileFromFile() failed");

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC input_element_descs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
        pso_desc.InputLayout = { input_element_descs, _countof(input_element_descs) };
        pso_desc.pRootSignature = input->m_root_signature.Get();
        pso_desc.VS = { reinterpret_cast<UINT8*>(vertex_shader->GetBufferPointer()), vertex_shader->GetBufferSize() };
        pso_desc.PS = { reinterpret_cast<UINT8*>(pixel_shader->GetBufferPointer()), pixel_shader->GetBufferSize() };
        pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        pso_desc.DepthStencilState.DepthEnable = FALSE;
        pso_desc.DepthStencilState.StencilEnable = FALSE;
        pso_desc.SampleMask = UINT_MAX;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso_desc.NumRenderTargets = 1;
        pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pso_desc.SampleDesc.Count = 1;
        result = input->m_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&input->m_pipeline_state));
        if (FAILED(result)) output("load_assets(): CreateGraphicsPipelineState() failed");
    }

    // Create the command list.
    {
    	HRESULT result = input->m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, input->m_command_allocators[input->m_frame_index].Get(), input->m_pipeline_state.Get(), IID_PPV_ARGS(&input->m_command_list));
    	if (FAILED(result)) output("load_assets(): CreateCommandList() failed");
	}

	// Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    {
    	HRESULT result = input->m_command_list->Close();
    	if (FAILED(result)) output("load_assets(): Close() failed");
    }

    // Create the vertex buffer.
    {
        // Define the geometry for a triangle.
        Vertex triangle_vertices[] =
        {
            { {   0.0f,  0.25f * input->sample.m_aspect_ratio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { {  0.25f, -0.25f * input->sample.m_aspect_ratio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * input->sample.m_aspect_ratio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        const UINT vertex_buffer_size = sizeof(triangle_vertices);

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
        HRESULT result = input->m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(vertex_buffer_size),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&input->m_vertex_buffer));
        if (FAILED(result)) output("load_assets(): CreateCommittedResource() failed");

        // Copy the triangle data to the vertex buffer.
        UINT8* p_vertex_data_begin;
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        result = input->m_vertex_buffer->Map(0, &readRange, reinterpret_cast<void**>(&p_vertex_data_begin));
        if (FAILED(result)) output("load_assets(): Map() failed");
        memcpy(p_vertex_data_begin, triangle_vertices, sizeof(triangle_vertices));
        input->m_vertex_buffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        input->m_vertex_buffer_view.BufferLocation = input->m_vertex_buffer->GetGPUVirtualAddress();
        input->m_vertex_buffer_view.StrideInBytes = sizeof(Vertex);
        input->m_vertex_buffer_view.SizeInBytes = vertex_buffer_size;
    }

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        HRESULT result = input->m_device->CreateFence(input->m_fence_values[input->m_frame_index], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&input->m_fence));
        if (FAILED(result)) output("load_assets(): CreateFence() failed");
        input->m_fence_values[input->m_frame_index]++;

        // Create an event handle to use for frame synchronization.
        input->m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (input->m_fence_event == nullptr) {
            HRESULT result = (HRESULT_FROM_WIN32(GetLastError()));
            if (FAILED(result)) output("load_assets(): GetLastError() failed");
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        dx12_wait_for_gpu(input);
    }
}

void dx_populate_command_list(dx_hello_triangle *input) {
	// Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    HRESULT result = input->m_command_allocators[input->m_frame_index]->Reset();
    if (FAILED(result)) output("dx_populate_command_list(): command allocator Reset() failed");

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    result = input->m_command_list->Reset(input->m_command_allocators[input->m_frame_index].Get(), input->m_pipeline_state.Get());
    if (FAILED(result)) output("dx_populate_command_list(): command list Reset() failed");

    // Set necessary state.
    input->m_command_list->SetGraphicsRootSignature(input->m_root_signature.Get());
    input->m_command_list->RSSetViewports(1, &input->m_viewport);
    input->m_command_list->RSSetScissorRects(1, &input->m_scissor_rect);

    // Indicate that the back buffer will be used as a render target.
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(input->m_render_targets[input->m_frame_index].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    input->m_command_list->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(input->m_rtv_heap->GetCPUDescriptorHandleForHeapStart(), input->m_frame_index, input->m_rtv_descriptor_size);
    input->m_command_list->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clear_color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    input->m_command_list->ClearRenderTargetView(rtvHandle, clear_color, 0, nullptr);
    input->m_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    input->m_command_list->IASetVertexBuffers(0, 1, &input->m_vertex_buffer_view);
    input->m_command_list->DrawInstanced(3, 1, 0, 0);

    // Indicate that the back buffer will now be used to present.
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(input->m_render_targets[input->m_frame_index].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    input->m_command_list->ResourceBarrier(1, &barrier);

    result = input->m_command_list->Close();
    if (FAILED(result)) output("dx_populate_command_list(): Close() failed");
}

void dx_on_render(dx_hello_triangle *input) {
	// Record all the commands we need to render the scene into the command list.
	dx_populate_command_list(input);

	// Execute the command list.
    ID3D12CommandList* pp_command_lists[] = { input->m_command_list.Get() };
    input->m_command_queue->ExecuteCommandLists(_countof(pp_command_lists), pp_command_lists);

    // Present the frame.
    HRESULT result = input->m_swap_chain->Present(1, 0);
    if (FAILED(result)) output("dx_on_render(): Present() failed");

    dx12_move_to_next_frame(input);
}

void dx_on_destroy(dx_hello_triangle *input) {
	// Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    dx12_wait_for_gpu(input);

    CloseHandle(input->m_fence_event);
}

//
// https://learn.microsoft.com/en-us/windows/win32/seccrypto/retrieving-error-messages
//
internal void
win32_print_error(DWORD err) {
    WCHAR buffer[512];  
    DWORD chars; 

    chars = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, 0, buffer, 512, NULL);

    // Display the error message, or generic text if not found.
    printf("Error value: %d Message: %ws\n", err, chars ? buffer : L"Error message not found." );
}

// query performance counter has a high resolution so it can be used to get the micro seconds between frames.
// SDL_GetTicks only returns the microseconds.
inline s64
win32_get_ticks() {
    LARGE_INTEGER result;
    if (!QueryPerformanceCounter(&result)) {
        win32_print_error(GetLastError());
    }
    return result.QuadPart;
}

// used to init global_perf_count_frequency
inline s64
win32_performance_frequency() {
    LARGE_INTEGER result;
    if (!QueryPerformanceFrequency(&result)) {
        win32_print_error(GetLastError());
    }
    return result.QuadPart;
}

inline r64
win32_get_seconds_elapsed(s64 start, s64 end) {
    r64 result = ((r64)(end - start) / (r64)global_perf_count_frequency);
    return result;
}

internal void
win32_process_pending_messages() {
    MSG message;
    while(PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
        switch(message.message) {
            case WM_QUIT: {
                win32_global_running = false;
            } break;

            default: {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            } break;
        }
    }
}

LRESULT CALLBACK main_window_callback(HWND window_handle, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;

    switch(message) {
        case WM_SIZE: {
            OutputDebugStringA("WM_SIZE\n"); 
        } break;

        case WM_ACTIVATEAPP: {
            output("WM_ACTIVATEAPP");
        } break;

        case WM_DESTROY: {
            //PostQuitMessage(0);
            //result = 0;

            win32_global_running = false;
        } break;

        case WM_PAINT:
        case WM_ERASEBKGND:
        default: {
            result = DefWindowProc(window_handle, message, wparam, lparam);
        } break;
    }

    return result;
}

void init_hello_triangle(dx_hello_triangle *triangle, UINT width, UINT height) {
    init_dx_sample(&triangle->sample, width, height);
    triangle->m_frame_index = 0;
    triangle->m_rtv_descriptor_size = 0;
    triangle->m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    triangle->m_scissor_rect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	WNDCLASS window_class = {};
	window_class.lpfnWndProc = main_window_callback;
	window_class.hInstance = hInstance;
	window_class.lpszClassName = "Direct3D12Basic";

	if (RegisterClassA(&window_class)) {
		HWND window_handle = CreateWindowExA(
			0, 
			window_class.lpszClassName,
			"Basic",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
            CW_USEDEFAULT,
            800,
            800,
            0,
            0,
            hInstance,
            0);

		if (window_handle) {

			platform_window_dimension dim;
			RECT client_rect;
    		GetClientRect(window_handle, &client_rect);
			dim.width = client_rect.right - client_rect.left;
    		dim.height = client_rect.bottom - client_rect.top;

			init_hello_triangle(&global_triangle, dim.width, dim.height);
			dx_load_pipeline(&global_triangle, window_handle);
			dx_load_assets(&global_triangle);
			global_triangle.initialized = true;

            global_perf_count_frequency = win32_performance_frequency();
            s64 last_frame_time = win32_get_ticks();

			while(win32_global_running) {
                win32_process_pending_messages();

                if (global_triangle.initialized)
                    dx_on_render(&global_triangle);

                s64 this_frame_time = win32_get_ticks();
                r64 fps = win32_get_seconds_elapsed(last_frame_time, this_frame_time);
                last_frame_time = this_frame_time;

                output("%f", 1.0f/fps);
			}

            dx_on_destroy(&global_triangle);
		} else {
			output("WinMain(): CreateWindowExA() failed");
		}
	} else {
		output("WinMain(): RegisterClassA() failed");
	}

	return 0;
}