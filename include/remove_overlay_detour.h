#pragma once

/*
CAUTION !!!!!!!!!!
For the detour to work, the memory layout of the OverlayInterface class within this header must be IDENTICAL to the OverlayInterface class
of the f4ee.dll, especially the std:: ... types must be the same (shared_ptr, multimap, etc.). This is only the case if you do a Release build,
Debug builds are CTD'ing !!!!!
*/

typedef uint32_t UniqueID;

class F4EEFixedString
{
public:
	F4EEFixedString() :
		m_internal() { m_hash = hash_lower(m_internal.c_str(), m_internal.size()); }
	F4EEFixedString(const char* str) :
		m_internal(str) { m_hash = hash_lower(m_internal.c_str(), m_internal.size()); }
	F4EEFixedString(const std::string& str) :
		m_internal(str) { m_hash = hash_lower(m_internal.c_str(), m_internal.size()); }
	F4EEFixedString(const RE::BSFixedString& str) :
		m_internal(str.c_str()) { m_hash = hash_lower(m_internal.c_str(), m_internal.size()); }

	bool operator==(const F4EEFixedString& x) const
	{
		if (m_internal.size() != x.m_internal.size())
			return false;

		if (_stricmp(m_internal.c_str(), x.m_internal.c_str()) == 0)
			return true;

		return false;
	}

	uint32_t length() { return (uint32_t)m_internal.size(); }

	operator RE::BSFixedString() const { return RE::BSFixedString(m_internal.c_str()); }
	RE::BSFixedString AsBSFixedString() const { return operator RE::BSFixedString(); }

	const char* c_str() const { return operator const char*(); }
	operator const char*() const { return m_internal.c_str(); }

	size_t hash_lower(const char* str, size_t count)
	{
		const size_t _FNV_offset_basis = 14695981039346656037ULL;
		const size_t _FNV_prime = 1099511628211ULL;

		size_t _Val = _FNV_offset_basis;
		for (size_t _Next = 0; _Next < count; ++_Next) { // fold in another byte
			_Val ^= (size_t)tolower(str[_Next]);
			_Val *= _FNV_prime;
		}
		return _Val;
	}

	size_t GetHash() const
	{
		return m_hash;
	}

protected:
	std::string m_internal;
	size_t m_hash;
};

typedef std::shared_ptr<F4EEFixedString> StringTableItem;
typedef std::weak_ptr<F4EEFixedString> WeakTableItem;

namespace std
{
	template <>
	struct hash<F4EEFixedString>
	{
		size_t operator()(const F4EEFixedString& x) const
		{
			return x.GetHash();
		}
	};
	template <>
	struct hash<StringTableItem>
	{
		size_t operator()(const StringTableItem& x) const
		{
			return x->GetHash();
		}
	};
}

class OverlayInterface : public RE::BSTEventSink<RE::TESObjectLoadedEvent>, public RE::BSTEventSink<RE::TESLoadGameEvent>
{
public:
	OverlayInterface() :
		m_highestUID(0) {}

	typedef uint32_t UniqueID;

	enum
	{
		kVersion1 = 1,
		kVersion2 = 2, // Version 2 now only saves uint32_t FormID instead of UInt64 Handle
		kSerializationVersion = kVersion2,
	};

	class OverlayData
	{
	public:
		enum Flags
		{
			kHasTintColor = (1 << 0),
			kHasOffsetUV = (1 << 1),
			kHasScaleUV = (1 << 2),
			kHasRemapIndex = (1 << 3)
		};

		OverlayData()
		{
			uid = 0;
			flags = 0;
			tintColor.r = 0.0f;
			tintColor.g = 0.0f;
			tintColor.b = 0.0f;
			tintColor.a = 0.0f;
			offsetUV.x = 0.0f;
			offsetUV.y = 0.0f;
			scaleUV.x = 1.0f;
			scaleUV.y = 1.0f;
			remapIndex = 0.50196f;
		}

		UniqueID uid;
		uint32_t flags;
		StringTableItem templateName;
		RE::NiColorA tintColor;
		RE::NiPoint2 offsetUV;
		RE::NiPoint2 scaleUV;
		float remapIndex;

		void UpdateFlags();

		void Save(const F4SE::SerializationInterface* intfc, uint32_t kVersion);
		bool Load(const F4SE::SerializationInterface* intfc, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
	};
	typedef std::shared_ptr<OverlayData> OverlayDataPtr;

	class PriorityMap : public std::multimap<int32_t, OverlayDataPtr>
	{
	public:
		void Save(const F4SE::SerializationInterface* intfc, uint32_t kVersion);
		bool Load(const F4SE::SerializationInterface* intfc, bool isFemale, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
	};
	typedef std::shared_ptr<PriorityMap> PriorityMapPtr;

	class OverlayMap : public std::unordered_map<uint32_t, PriorityMapPtr>
	{
	public:
		void Save(const F4SE::SerializationInterface* intfc, uint32_t kVersion);
		bool Load(const F4SE::SerializationInterface* intfc, bool isFemale, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
	};

	class OverlayTemplate
	{
	public:
		OverlayTemplate() :
			playable(false), sort(0), transformable(false), tintable(false) {}

		typedef std::unordered_map<uint32_t, std::pair<F4EEFixedString, bool>> MaterialMap;

		F4EEFixedString displayName;
		MaterialMap slotMaterial;
		bool playable;
		bool transformable;
		bool tintable;
		int32_t sort;
	};
	typedef std::shared_ptr<OverlayTemplate> OverlayTemplatePtr;

	virtual void Save(const F4SE::SerializationInterface* intfc, uint32_t kVersion);
	virtual bool Load(const F4SE::SerializationInterface* intfc, uint32_t kVersion, const std::unordered_map<uint32_t, StringTableItem>& stringTable);
	virtual void Revert();

	virtual void LoadOverlayMods();
	virtual void ClearMods();
	virtual bool LoadOverlayTemplates(const std::string& filePath);

	virtual UniqueID AddOverlay(RE::Actor* actor, bool isFemale, int32_t priority, const F4EEFixedString& templateName, const RE::NiColorA& tintColor, const RE::NiPoint2& offsetUV, const RE::NiPoint2& scaleUV);
	virtual bool RemoveOverlay(RE::Actor* actor, bool isFemale, UniqueID uid);
	virtual bool RemoveAll(RE::Actor* actor, bool isFemale);
	virtual bool ReorderOverlay(RE::Actor* actor, bool isFemale, UniqueID uid, int32_t newPriority);

	virtual bool ForEachOverlay(RE::Actor* actor, bool isFemale, std::function<void(int32_t, const OverlayDataPtr&)> functor);
	virtual bool ForEachOverlayBySlot(RE::Actor* actor, bool isFemale, uint32_t slotIndex, std::function<void(int32_t, const OverlayDataPtr&, const F4EEFixedString&, bool)> functor);

	virtual void ForEachOverlayTemplate(bool isFemale, std::function<void(const F4EEFixedString&, const OverlayTemplatePtr&)> functor);

	virtual bool UpdateOverlays(RE::Actor* actor);
	virtual bool UpdateOverlay(RE::Actor* actor, uint32_t slotIndex);

	virtual void CloneOverlays(RE::Actor* source, RE::Actor* target);

	virtual UniqueID GetNextUID();

	virtual RE::NiNode* GetOverlayRoot(RE::Actor* actor, RE::NiNode* rootNode, bool createIfNecessary = true);

	virtual const OverlayTemplatePtr GetTemplateByName(bool isFemale, const F4EEFixedString& name);
	virtual const OverlayDataPtr GetOverlayByUID(UniqueID uid);

	std::pair<int32_t, OverlayDataPtr> GetActorOverlayByUID(RE::Actor* actor, bool isFemale, UniqueID uid);

	bool HasSkinChildren(RE::NiAVObject* slot);
	void LoadMaterialData(RE::TESNPC* npc, RE::BSTriShape* shape, const F4EEFixedString& material, bool effect, const OverlayDataPtr& overlayData);

	void DestroyOverlaySlot(RE::Actor* actor, RE::NiNode* overlayHolder, uint32_t slotIndex);
	bool UpdateOverlays(RE::Actor* actor, RE::NiNode* rootNode, RE::NiAVObject* object, uint32_t slotIndex);

protected:
	friend class OverlayTemplate;
	friend class PriorityMap;
	friend class OverlayData;

	RE::BSSpinLock m_overlayLock;
	OverlayMap m_overlays[2];
	std::vector<UniqueID> m_freeIndices;
	std::unordered_map<UniqueID, OverlayDataPtr> m_dataMap;
	UniqueID m_highestUID;
	std::unordered_map<F4EEFixedString, OverlayTemplatePtr> m_overlayTemplates[2];
	friend bool HookedRemoveOverlay(OverlayInterface* OverlayInterface, RE::Actor* actor, bool isFemale, UniqueID uid);
};

bool HookedRemoveOverlay(OverlayInterface* overlayInterface, RE::Actor* actor, bool isFemale, UniqueID uid)
{
	RE::BSSpinLock locker(overlayInterface->m_overlayLock);
	// logger::info("HookedRemoveOverlay function called");
	auto hit = overlayInterface->m_overlays[isFemale ? 1 : 0].find(actor->formID);
	if (hit == overlayInterface->m_overlays[isFemale ? 1 : 0].end())
		return false;
	// logger::info("HookedRemoveOverlay #1");
	OverlayInterface::PriorityMapPtr priorityMap = hit->second;
	if (!priorityMap)
		return false;
	// logger::info("HookedRemoveOverlay #2");
	for (auto it = priorityMap->begin(); it != priorityMap->end(); ++it) {
		OverlayInterface::OverlayDataPtr overlayPtr = it->second;
		if (!overlayPtr)
			continue;

		// logger::info("HookedRemoveOverlay #3");
		if (overlayPtr->uid == uid) {
			overlayInterface->m_dataMap.erase(overlayPtr->uid);
			logger::info("HookedRemoveOverlay #4");
			overlayInterface->m_freeIndices.push_back(overlayPtr->uid);
			logger::info("Remove overlay id has been added to 'm_freeIndices'");
			priorityMap->erase(it);
			return true;
		}
	}

	// logger::info("HookedRemoveOverlay #5");
	return false;
}
