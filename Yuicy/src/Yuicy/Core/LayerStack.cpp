#include "pch.h"
#include "Yuicy/Core/LayerStack.h"

namespace Yuicy {

	LayerStack::~LayerStack()  // 图层在栈中长期存在，结束时销毁
	{
		for (Layer* layer : _Layers)
		{
			layer->OnDetach();
			delete layer;
		}
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		_Layers.emplace(_Layers.begin() + _LayerInsertIndex, layer);
		_LayerInsertIndex++;
	}

	void LayerStack::PushOverlay(Layer* overlay)
	{
		_Layers.emplace_back(overlay);
	}

	void LayerStack::PopLayer(Layer* layer)  // 弹出时Layer并不会被delete
	{
		auto it = std::find(_Layers.begin(), _Layers.begin() + _LayerInsertIndex, layer);
		if (it != _Layers.begin() + _LayerInsertIndex)
		{
			layer->OnDetach();
			_Layers.erase(it);
			_LayerInsertIndex--;
		}
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto it = std::find(_Layers.begin() + _LayerInsertIndex, _Layers.end(), overlay);
		if (it != _Layers.end())
		{
			overlay->OnDetach();
			_Layers.erase(it);
		}
	}

}
