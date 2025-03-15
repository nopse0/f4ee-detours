#include "detourxs/detourxs.h"
#include "remove_overlay_detour.h"



uint64_t checksum(const std::string& filename)
{
	std::ifstream file(filename, std::ios::binary);
	if (!file) {
		logger::warn("Error opening file: {}", filename);
		return 0;
	}

	std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});

	uint64_t sum = 0;
	for (uint8_t byte : buffer) {
		sum += byte;
	}

	return sum;
}

namespace
{
	DetourXS removeOverlayDetour;
	using RemoveOverlay_t = bool (*)(OverlayInterface*, bool, UniqueID);
	RemoveOverlay_t removeOverlayTrampoline = nullptr;

	void install_hook()
	{

		typedef bool (*RemoveOverlay_t)(OverlayInterface*, bool, UniqueID);

		auto sum = checksum("Data/F4SE/Plugins/f4ee.dll");
		// logger::info("f4ee.dll checksum = {}", sum);
		// f4ee.dll v1.6.20 checksum is: 94335105
		if (sum == 94335105LL) {
			// We don't need the detour later on, but the destructor mustn't be called
			uintptr_t baseAddr = reinterpret_cast<uintptr_t>(GetModuleHandleA("f4ee.dll"));
			// Thanks to Evi1Panda for finding out the offset of the RemoveOverlay function
			if (baseAddr && !removeOverlayDetour.Create(reinterpret_cast<LPVOID>(baseAddr + 0x420D0), &HookedRemoveOverlay)) {
				logger::info("Failed to create OverlayInterface::RemoveOverlay detour!");
			}
			else {
				logger::info("Created OverlayInterface::RemoveOverlay detour successfully!");
				// We don't need the trampoline, but keeping it in memory does no harm
				removeOverlayTrampoline = reinterpret_cast<RemoveOverlay_t>(removeOverlayDetour.GetTrampoline());
			}
		}
	}
}

void F4SEAPI messaging_hook(F4SE::MessagingInterface::Message* a_message)
{
	switch (a_message->type) {
		case F4SE::MessagingInterface::kPostPostLoad:
			if (REL::Module::IsF4()) {
				install_hook();
			}
			break;
	}
}

F4SE_EXPORT constinit auto F4SEPlugin_Version = []() noexcept {
	auto data = F4SE::PluginVersionData();

	data.AuthorName(Plugin::AUTHOR);
	data.PluginName(Plugin::NAME);
	data.PluginVersion(Plugin::VERSION);

	data.UsesAddressLibrary(true);
	data.IsLayoutDependent(true);
	data.UsesSigScanning(false);
	data.HasNoStructUse(false);

	data.CompatibleVersions({ F4SE::RUNTIME_LATEST_NG });
	return data;
}();

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
	logger::info("{} v{}"sv, Plugin::NAME, Plugin::VERSION);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION.major();

	if (a_f4se->IsEditor()) {
		logger::critical("Loading in editor is not supported"sv);
		return false;
	}

	const auto version = a_f4se->RuntimeVersion();
	if (version < REL::Relocate(F4SE::RUNTIME_LATEST_OG, F4SE::RUNTIME_LATEST_NG)) {
		logger::critical("Unsupported runtime v{}"sv, version.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	logger::info("Loaded"sv);

	const auto runtimeVer = REL::Module::get().version();
	logger::info("Fallout 4 v{}.{}.{}"sv, runtimeVer[0], runtimeVer[1], runtimeVer[2]);
	const auto messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(messaging_hook)) {
		return false;
	}

	return true;
}
