#define NAPI_VERSION 3
#include <node_api.h>
#include <vector>
#include <mutex>
#include <atomic>

#define NAPI_CALL(env, call)                                                   \
  do {                                                                         \
    napi_status status = (call);                                               \
    if (status != napi_ok) {                                                   \
      const napi_extended_error_info *error_info = NULL;                       \
      napi_get_last_error_info((env), &error_info);                            \
      bool is_pending;                                                         \
      napi_is_exception_pending((env), &is_pending);                           \
      if (!is_pending) {                                                       \
        const char *message = (error_info->error_message == NULL)              \
                                  ? "empty error message"                      \
                                  : error_info->error_message;                 \
        napi_throw_error((env), NULL, message);                                \
        return NULL;                                                           \
      }                                                                        \
    }                                                                          \
  } while (0)

#ifdef __APPLE__
#include "progress_bar_macos.h"
#endif

struct ProgressBarContext {
    void* handle;
    std::atomic<bool> isValid{true};
};

static std::vector<void*> active_handles;
static std::mutex handles_mutex;

static void FinalizeProgressBar(napi_env env, void* finalize_data, void* finalize_hint) {
    ProgressBarContext* context = static_cast<ProgressBarContext*>(finalize_data);
    if (context && context->isValid.exchange(false)) {
        if (context->handle) {
            CloseProgressBarMacOS(context->handle);
            context->handle = nullptr;
        }
        delete context;
    }
}

static void CleanupProgressBars(void* arg) {
    std::lock_guard<std::mutex> lock(handles_mutex);
    for (void* handle : active_handles) {
        if (handle) {
            CloseProgressBarMacOS(handle);
        }
    }
    active_handles.clear();
}

static napi_value ShowProgressBar(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < 3) {
        napi_throw_error(env, nullptr, "Wrong number of arguments");
        return nullptr;
    }

    size_t title_size;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], nullptr, 0, &title_size));
    char* title = new char[title_size + 1];
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], title, title_size + 1, nullptr));

    size_t message_size;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], nullptr, 0, &message_size));
    char* message = new char[message_size + 1];
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], message, message_size + 1, nullptr));

    size_t style_size;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[2], nullptr, 0, &style_size));
    char* style = new char[style_size + 1];
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[2], style, style_size + 1, nullptr));

    ProgressBarContext* context = new ProgressBarContext();
#ifdef __APPLE__
    context->handle = ShowProgressBarMacOS(title, message, style);
#endif
    delete[] title;
    delete[] message;
    delete[] style;

    napi_value external;
    napi_status status = napi_create_external(env, context, FinalizeProgressBar, nullptr, &external);
    if (status != napi_ok) {
        delete context;
        napi_throw_error(env, nullptr, "Failed to create external");
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(handles_mutex);
        active_handles.push_back(context->handle);
    }

    return external;
}

static napi_value UpdateProgress(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < 2) {
        napi_throw_error(env, nullptr, "Wrong number of arguments");
        return nullptr;
    }

    void* data;
    NAPI_CALL(env, napi_get_value_external(env, args[0], &data));
    ProgressBarContext* context = static_cast<ProgressBarContext*>(data);

    if (!context || !context->isValid.load() || !context->handle) {
        return nullptr;
    }

    int32_t progress;
    NAPI_CALL(env, napi_get_value_int32(env, args[1], &progress));

    if (argc >= 3) {
        napi_valuetype value_type;
        NAPI_CALL(env, napi_typeof(env, args[2], &value_type));
        
        if (value_type == napi_string) {
            size_t message_length;
            NAPI_CALL(env, napi_get_value_string_utf8(env, args[2], nullptr, 0, &message_length));
            char* message_buffer = new char[message_length + 1];
            NAPI_CALL(env, napi_get_value_string_utf8(env, args[2], message_buffer, message_length + 1, nullptr));
            
            UpdateProgressBarMacOS(context->handle, progress, message_buffer);
            
            delete[] message_buffer;
        } else {
            UpdateProgressBarMacOS(context->handle, progress, nullptr);
        }
    } else {
        UpdateProgressBarMacOS(context->handle, progress, nullptr);
    }

    return nullptr;
}

static napi_value CloseProgress(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < 1) {
        napi_throw_error(env, nullptr, "Wrong number of arguments");
        return nullptr;
    }

    void* data;
    NAPI_CALL(env, napi_get_value_external(env, args[0], &data));
    ProgressBarContext* context = static_cast<ProgressBarContext*>(data);

    if (context && context->isValid.exchange(false)) {
        if (context->handle) {
            CloseProgressBarMacOS(context->handle);
            context->handle = nullptr;
        }
    }

    return nullptr;
}

NAPI_MODULE_INIT() {
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_add_env_cleanup_hook(env, CleanupProgressBars, nullptr);

#ifdef __APPLE__
    napi_value show_fn, update_fn, close_fn;
    NAPI_CALL(env, napi_create_function(env, "showProgressBar", NAPI_AUTO_LENGTH, 
                                       ShowProgressBar, NULL, &show_fn));
    NAPI_CALL(env, napi_create_function(env, "updateProgress", NAPI_AUTO_LENGTH, 
                                       UpdateProgress, NULL, &update_fn));
    NAPI_CALL(env, napi_create_function(env, "closeProgress", NAPI_AUTO_LENGTH, 
                                       CloseProgress, NULL, &close_fn));
    NAPI_CALL(env, napi_set_named_property(env, result, "showProgressBar", show_fn));
    NAPI_CALL(env, napi_set_named_property(env, result, "updateProgress", update_fn));
    NAPI_CALL(env, napi_set_named_property(env, result, "closeProgress", close_fn));
#endif
    return result;
}
