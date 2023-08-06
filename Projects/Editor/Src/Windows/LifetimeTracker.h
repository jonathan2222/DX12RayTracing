#pragma once

#include "EditorWindow.h"

namespace RSE
{
	class LifetimeTracker : public EditorWindow
	{
	public:
		LifetimeTracker(const std::string& name, bool enabled) : EditorWindow(name, enabled) {}

		virtual void Render() override;

	private:
	};
}