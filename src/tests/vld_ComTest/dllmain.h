class CComTestModule : public ATL::CAtlDllModuleT< CComTestModule >
{
public :
	DECLARE_LIBID(LIBID_ComTestLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_COMTEST, "{06DD8E8C-4587-41A0-8D5A-B3A87883B158}")
};

extern class CComTestModule _AtlModule;
