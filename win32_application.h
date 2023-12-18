struct dx_sample {
	// viewport dimensions
	UINT m_width;
	UINT m_height;
	float m_aspect_ratio;

	bool m_use_warp_device; // Adapter info
};

struct dx_hello_triangle {
	bool initialized;

	dx_sample sample;
	
	static const UINT frame_count = 2;
	
	// Pipeline objects
	CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissor_rect;
	ComPtr<IDXGISwapChain3> m_swap_chain;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12Resource> m_render_targets[frame_count];
	ComPtr<ID3D12CommandAllocator> m_command_allocators[frame_count];
	ComPtr<ID3D12CommandQueue> m_command_queue;
	ComPtr<ID3D12RootSignature> m_root_signature;
	ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
	ComPtr<ID3D12PipelineState> m_pipeline_state;
	ComPtr<ID3D12GraphicsCommandList> m_command_list;
	UINT m_rtv_descriptor_size;

	// App resources.
    ComPtr<ID3D12Resource> m_vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view;

	// Synchronization objects
	UINT m_frame_index;
    HANDLE m_fence_event;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fence_values[frame_count];
};

struct Vertex
{
    v3 position;
    v4 color;
};

struct platform_window_dimension {
	s32 width;
	s32 height;
};

global s64 global_perf_count_frequency;
global dx_hello_triangle global_triangle = {};
global b32 win32_global_running = true;