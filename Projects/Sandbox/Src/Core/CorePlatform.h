#pragma once

namespace RS
{
	class CorePlatform
	{
	public:
		RS_DEFAULT_ABSTRACT_CLASS(CorePlatform)

		static std::shared_ptr<CorePlatform> Get();

		std::string GetComputerNameStr() const;
		std::string GetUserNameStr() const;

		std::string GetConfigurationAsStr() const;
	};
}