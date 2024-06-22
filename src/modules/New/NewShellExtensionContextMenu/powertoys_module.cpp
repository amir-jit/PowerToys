#include "pch.h"

#include <filesystem>
#include <string>

#include <winrt/Windows.Data.Json.h>

#include <common/SettingsAPI/settings_objects.h>
#include <common/utils/gpo.h>
#include <common/utils/process_path.h>
#include <common/utils/resources.h>
#include <interface/powertoy_module_interface.h>

#include "constants.h"
#include "settings.h"
#include "trace.h"
#include "new_utilities.h"
#include "Generated Files/resource.h"

// Note: Settings are managed via Settings and UI Settings
class NewModule : public PowertoyModuleIface
{
public:
    NewModule()
    {
        init_settings();
    }

    virtual const wchar_t* get_name() override
    {
        static const std::wstring localized_context_menu_item =
            GET_RESOURCE_STRING_FALLBACK(IDS_CONTEXT_MENU_ITEM_NEW, L"New+");

        return localized_context_menu_item.c_str();
    }

    virtual const wchar_t* get_key() override
    {
        // This setting key must match EnabledModules.cs [JsonPropertyName("New")]
        return newplus::constants::non_localizable::powertoy_key;
    }

    virtual powertoys_gpo::gpo_rule_configured_t gpo_policy_enabled_configuration() override
    {
        return powertoys_gpo::getConfiguredNewEnabledValue();
    }

    virtual bool get_config(_Out_ PWSTR buffer, _Out_ int* buffer_size) override
    {
        // Not implemented as Settings are propagating via json
        return true;
    }

    virtual void set_config(PCWSTR config) override
    {
        // The following just checks to see if the Template Location was changed for metrics purposes
        // Note: We are not saving the settings here and instead relying on read/write of json in Settings App .cs codepaths
        try
        {
            json::JsonObject config_as_json = json::JsonValue::Parse(winrt::to_hstring(config)).GetObjectW();

            const auto latest_location_value = config_as_json.GetNamedString(newplus::constants::non_localizable::settings_json_key_template_location).data();
            const auto existing_location_value = NewSettingsInstance().GetTemplateLocation();

            if (!newplus::utilities::wstring_same_when_comparing_ignore_case(latest_location_value, existing_location_value))
            {
                Trace::EventChangedTemplateLocation();
            }
        }
        catch (std::exception& e)
        {
            Logger::error("Configuration parsing failed: {}", std::string{ e.what() });
        }
    }

    virtual bool is_enabled_by_default() const override
    { 
        return false; 
    }

    virtual void enable() override
    {
        Logger::info("New+ enabled via Settings UI");

        newplus::utilities::register_msix_package();

        powertoy_new_enabled = true;
    }

    virtual void disable() override
    {
        Logger::info("New+ disabled via Settings UI");

        powertoy_new_enabled = false;
    }

    virtual bool is_enabled() override
    {
        Logger::info("New+ is enabled");

        return powertoy_new_enabled;
    }

    virtual void hide_file_extension(bool hide_file_extension)
    {
        Logger::info("New+ hide file extension {}", hide_file_extension);
    }

    virtual void template_location(std::wstring path_location)
    {
        Logger::info("New+ template location");
    }

    virtual void destroy() override
    {
        delete this;
    }

private:
    bool powertoy_new_enabled = false;

    void init_settings()
    {
        powertoy_new_enabled = NewSettingsInstance().GetEnabled();
    }
};

extern "C" __declspec(dllexport) PowertoyModuleIface* __cdecl powertoy_create()
{
    return new NewModule();
}
