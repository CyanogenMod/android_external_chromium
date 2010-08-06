ifneq ($(TARGET_SIMULATOR),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc

LOCAL_ARM_MODE := arm

LOCAL_MODULE := libchromium_net
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
INTERMEDIATES := $(call local-intermediates-dir)

LOCAL_SRC_FILES := \
	googleurl/src/gurl.cc \
	googleurl/src/url_canon_etc.cc \
	googleurl/src/url_canon_fileurl.cc \
	googleurl/src/url_canon_host.cc \
	googleurl/src/url_canon_icu.cc \
	googleurl/src/url_canon_internal.cc \
	googleurl/src/url_canon_ip.cc \
	googleurl/src/url_canon_mailtourl.cc \
	googleurl/src/url_canon_path.cc \
	googleurl/src/url_canon_pathurl.cc \
	googleurl/src/url_canon_query.cc \
	googleurl/src/url_canon_relative.cc \
	googleurl/src/url_canon_stdurl.cc \
	googleurl/src/url_parse.cc \
	googleurl/src/url_parse_file.cc \
	googleurl/src/url_util.cc \
	\
	app/sql/connection.cc \
	app/sql/meta_table.cc \
	app/sql/statement.cc \
	app/sql/transaction.cc \
	\
	base/at_exit.cc \
    base/base64.cc \
    base/cancellation_flag.cc \
    base/condition_variable_posix.cc \
    base/debug_util_posix.cc \
    base/field_trial.cc \
    base/file_descriptor_shuffle.cc \
    base/file_path.cc \
    base/file_util.cc \
    base/file_util_android.cc \
    base/file_util_posix.cc \
    base/histogram.cc \
    base/lazy_instance.cc \
    base/lock_impl_posix.cc \
    base/logging.cc \
    base/message_loop.cc \
    base/message_loop_proxy_impl.cc \
    base/message_pump_libevent.cc \
    base/message_pump_default.cc \
    base/md5.cc \
    base/native_library_linux.cc \
    base/pickle.cc \
    base/platform_file_posix.cc \
    base/platform_thread_posix.cc \
    base/process_util.cc \
    base/process_util_linux.cc \
    base/process_util_posix.cc \
    base/rand_util.cc \
    base/rand_util_posix.cc \
    base/ref_counted.cc \
    base/safe_strerror_posix.cc \
    base/sha1.cc \
    base/sha2.cc \
    base/stats_table.cc \
    base/string_piece.cc \
    base/string_util.cc \
    base/string16.cc \
    base/sys_info_posix.cc \
    base/sys_string_conversions_linux.cc \
    base/task.cc \
    base/thread.cc \
    base/thread_local_posix.cc \
    base/thread_local_storage_posix.cc \
    base/time.cc \
    base/time_posix.cc \
    base/timer.cc \
    base/trace_event.cc \
    base/tracked.cc \
    base/waitable_event_posix.cc \
    base/utf_offset_string_conversions.cc \
    base/utf_string_conversions.cc \
    base/utf_string_conversion_utils.cc \
    base/values.cc \
    base/weak_ptr.cc \
    base/worker_pool_linux.cc \
    \
    base/i18n/file_util_icu.cc \
    base/i18n/time_formatting.cc \
    \
    base/json/json_reader.cc \
    base/json/json_writer.cc \
    base/json/string_escape.cc \
    \
    base/i18n/icu_string_conversions.cc \
    \
    base/third_party/dmg_fp/dtoa.cc \
    base/third_party/dmg_fp/g_fmt.cc \
    \
    base/third_party/icu/icu_utf.cc \
    \
    base/third_party/nspr/prtime.cc \
    \
    base/third_party/nss/sha512.cc \
    \
    chrome/browser/net/sqlite_persistent_cookie_store.cc \
    \
    third_party/modp_b64/modp_b64.cc \
    \
    net/base/address_list.cc \
    net/base/address_list_net_log_param.cc \
    net/base/capturing_net_log.cc \
    net/base/connection_type_histograms.cc \
    net/base/cookie_monster.cc \
    net/base/data_url.cc \
    net/base/directory_lister.cc \
    net/base/dns_util.cc \
    net/base/escape.cc \
    net/base/file_stream_posix.cc \
    net/base/filter.cc \
    net/base/forwarding_net_log.cc \
    net/base/gzip_filter.cc \
    net/base/gzip_header.cc \
    net/base/host_cache.cc \
    net/base/host_mapping_rules.cc \
    net/base/host_port_pair.cc \
    net/base/host_resolver.cc \
    net/base/host_resolver_impl.cc \
    net/base/host_resolver_proc.cc \
    net/base/https_prober.cc \
    net/base/io_buffer.cc \
    net/base/load_log.cc \
    net/base/mime_util.cc \
    net/base/net_errors.cc \
    net/base/net_log.cc \
    net/base/net_module.cc \
    net/base/net_util.cc \
    net/base/net_util_posix.cc \
    net/base/network_change_notifier.cc \
    net/base/network_change_notifier_linux.cc \
    net/base/network_change_notifier_netlink_linux.cc \
    net/base/registry_controlled_domain.cc \
    net/base/sdch_manager.cc \
    net/base/sdch_filter.cc \
    net/base/ssl_client_auth_cache.cc \
    net/base/ssl_config_service.cc \
    net/base/transport_security_state.cc \
    net/base/upload_data.cc \
    net/base/upload_data_stream.cc \
    net/base/x509_certificate.cc \
    net/base/x509_certificate_openssl.cc \
    \
    net/disk_cache/addr.cc \
    net/disk_cache/backend_impl.cc \
    net/disk_cache/bitmap.cc \
    net/disk_cache/block_files.cc \
    net/disk_cache/cache_util_posix.cc \
    net/disk_cache/entry_impl.cc \
    net/disk_cache/eviction.cc \
    net/disk_cache/file_lock.cc \
    net/disk_cache/file_posix.cc \
    net/disk_cache/hash.cc \
    net/disk_cache/in_flight_backend_io.cc \
    net/disk_cache/in_flight_io.cc \
    net/disk_cache/mapped_file_posix.cc \
    net/disk_cache/mem_backend_impl.cc \
    net/disk_cache/mem_entry_impl.cc \
    net/disk_cache/mem_rankings.cc \
    net/disk_cache/rankings.cc \
    net/disk_cache/stats.cc \
    net/disk_cache/stats_histogram.cc \
    net/disk_cache/sparse_control.cc \
    net/disk_cache/trace.cc \
    \
    net/spdy/spdy_framer.cc \
    net/spdy/spdy_frame_builder.cc \
    net/spdy/spdy_http_stream.cc \
    net/spdy/spdy_io_buffer.cc \
    net/spdy/spdy_network_transaction.cc \
    net/spdy/spdy_session.cc \
    net/spdy/spdy_session_pool.cc \
    net/spdy/spdy_settings_storage.cc \
    net/spdy/spdy_stream.cc \
    \
    net/http/des.cc \
    net/http/http_alternate_protocols.cc \
    net/http/http_auth.cc \
    net/http/http_auth_cache.cc \
    net/http/http_auth_controller.cc \
    net/http/http_auth_gssapi_posix.cc \
    net/http/http_auth_handler.cc \
    net/http/http_auth_handler_basic.cc \
    net/http/http_auth_handler_digest.cc \
    net/http/http_auth_handler_factory.cc \
    net/http/http_auth_handler_negotiate.cc \
    net/http/http_auth_handler_ntlm.cc \
    net/http/http_auth_handler_ntlm_portable.cc \
    net/http/http_basic_stream.cc \
    net/http/http_byte_range.cc \
    net/http/http_cache.cc \
    net/http/http_cache_transaction.cc \
    net/http/http_chunked_decoder.cc \
    net/http/http_network_layer.cc \
    net/http/http_network_session.cc \
    net/http/http_network_transaction.cc \
    net/http/http_proxy_client_socket.cc \
    net/http/http_proxy_client_socket_pool.cc \
    net/http/http_request_headers.cc \
    net/http/http_response_headers.cc \
    net/http/http_response_info.cc \
    net/http/http_stream_parser.cc \
    net/http/http_util.cc \
    net/http/http_util_icu.cc \
    net/http/http_vary_data.cc \
    net/http/md4.cc \
    net/http/partial_data.cc \
    \
    net/proxy/init_proxy_resolver.cc \
    net/proxy/proxy_bypass_rules.cc \
    net/proxy/proxy_config.cc \
    net/proxy/proxy_info.cc \
    net/proxy/proxy_list.cc \
    net/proxy/multi_threaded_proxy_resolver.cc \
    net/proxy/proxy_resolver_js_bindings.cc \
    net/proxy/proxy_resolver_script_data.cc \
    net/proxy/proxy_resolver_v8.cc \
    net/proxy/proxy_script_fetcher.cc \
    net/proxy/proxy_server.cc \
    net/proxy/proxy_service.cc \
    net/proxy/sync_host_resolver_bridge.cc \
    \
    net/socket/client_socket_handle.cc \
    net/socket/client_socket_factory.cc \
    net/socket/client_socket_pool.cc \
    net/socket/client_socket_pool_base.cc \
    net/socket/client_socket_pool_histograms.cc \
    net/socket/socks_client_socket.cc \
    net/socket/socks_client_socket_pool.cc \
    net/socket/socks5_client_socket.cc \
    net/socket/ssl_client_socket_openssl.cc \
    net/socket/ssl_client_socket_pool.cc \
    net/socket/tcp_client_socket_libevent.cc \
    net/socket/tcp_client_socket_pool.cc \
    \
    net/url_request/url_request.cc \
    net/url_request/url_request_file_job.cc \
    net/url_request/url_request_file_dir_job.cc \
    net/url_request/url_request_http_job.cc \
    net/url_request/url_request_error_job.cc \
    net/url_request/url_request_job.cc \
    net/url_request/url_request_job_manager.cc \
    net/url_request/url_request_job_tracker.cc \
    net/url_request/url_request_netlog_params.cc \
    net/url_request/url_request_redirect_job.cc \
    \
    sdch/open-vcdiff/src/addrcache.cc \
    sdch/open-vcdiff/src/blockhash.cc \
    sdch/open-vcdiff/src/codetable.cc \
    sdch/open-vcdiff/src/encodetable.cc \
    sdch/open-vcdiff/src/decodetable.cc \
    sdch/open-vcdiff/src/headerparser.cc \
    sdch/open-vcdiff/src/instruction_map.cc \
    sdch/open-vcdiff/src/logging.cc \
    sdch/open-vcdiff/src/varint_bigendian.cc \
    sdch/open-vcdiff/src/vcdecoder.cc \
    sdch/open-vcdiff/src/vcdiffengine.cc \
    sdch/open-vcdiff/src/vcencoder.cc \
    \
    third_party/libevent/event.c \
    third_party/libevent/evutil.c \
    third_party/libevent/epoll.c \
    third_party/libevent/log.c \
    third_party/libevent/poll.c \
    third_party/libevent/select.c \
    third_party/libevent/signal.c

# AutoFill++ source files.
LOCAL_SRC_FILES += \
    android/autofill/android_url_request_context_getter.cc \
    android/autofill/profile_android.cc \
    \
    base/base_paths.cc \
    base/base_paths_posix.cc \
    base/env_var.cc \
    base/path_service.cc \
    \
    chrome/browser/autofill/address.cc \
    chrome/browser/autofill/address_field.cc \
    chrome/browser/autofill/autofill_download.cc \
    chrome/browser/autofill/autofill_field.cc \
    chrome/browser/autofill/autofill_manager.cc \
    chrome/browser/autofill/autofill_profile.cc \
    chrome/browser/autofill/autofill_type.cc \
    chrome/browser/autofill/autofill_xml_parser.cc \
    chrome/browser/autofill/contact_info.cc \
    chrome/browser/autofill/credit_card.cc \
    chrome/browser/autofill/credit_card_field.cc \
    chrome/browser/autofill/fax_field.cc \
    chrome/browser/autofill/form_structure.cc \
    chrome/browser/autofill/form_field.cc \
    chrome/browser/autofill/form_group.cc \
    chrome/browser/autofill/name_field.cc \
    chrome/browser/autofill/personal_data_manager.cc \
    chrome/browser/autofill/phone_field.cc \
    chrome/browser/autofill/phone_number.cc \
    \
    chrome/browser/config_dir_policy_provider.cc \
    chrome/browser/configuration_policy_provider.cc \
    chrome/browser/pref_service.cc \
    chrome/browser/pref_value_store.cc \
    \
    chrome/common/json_value_serializer.cc \
    chrome/common/pref_names.cc \
    chrome/common/url_constants.cc \
    \
    chrome/common/net/url_fetcher.cc \
    chrome/common/net/url_fetcher_protect.cc \
    chrome/common/net/url_request_context_getter.cc \
    \
    third_party/libjingle/overrides/talk/xmllite/qname.cc \
    third_party/libjingle/source/talk/xmllite/xmlbuilder.cc \
    third_party/libjingle/source/talk/xmllite/xmlconstants.cc \
    third_party/libjingle/source/talk/xmllite/xmlelement.cc \
    third_party/libjingle/source/talk/xmllite/xmlnsstack.cc \
    third_party/libjingle/source/talk/xmllite/xmlparser.cc \
    third_party/libjingle/source/talk/xmllite/xmlprinter.cc

# external/chromium/android is a directory to intercept stl headers that we do
# not support yet.
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/android \
    $(LOCAL_PATH)/chrome \
    $(LOCAL_PATH)/chrome/browser \
    $(LOCAL_PATH)/sdch/linux \
    $(LOCAL_PATH)/sdch/open-vcdiff/src \
    $(LOCAL_PATH)/third_party/libevent/compat \
    external/expat \
    external/icu4c/common \
    external/icu4c/i18n \
    external/openssl/include \
    external/skia \
    external/sqlite/dist \
    external/webkit/WebKit/chromium \
    external/webkit/WebKit/android \
    external/zlib \
    external \
    bionic \
    bionic/libc/include \
    $(LOCAL_PATH)/base/third_party/dmg_fp \
    $(LOCAL_PATH)/third_party/icu/public/common \
    $(LOCAL_PATH)/third_party/libevent/android \
    $(LOCAL_PATH)/third_party/libevent \
    $(LOCAL_PATH)/third_party/libjingle/overrides \
    $(LOCAL_PATH)/third_party/libjingle/source \
    vendor/google/libraries/autofill

# Chromium uses several third party libraries and headers that are already
# present on Android, but in different include paths. Generate a set of
# forwarding headers in the location that Chromium expects.

THIRD_PARTY = $(INTERMEDIATES)/third_party
SCRIPT := $(LOCAL_PATH)/android/generateAndroidForwardingHeader.pl
CHECK_INTERNAL_HEADER_SCRIPT := $(LOCAL_PATH)/android/haveAutofillInternal.pl

GEN := $(THIRD_PARTY)/expat/files/lib/expat.h
$(GEN): $(SCRIPT)
$(GEN):
	perl $(SCRIPT) $@ "lib/expat.h"
LOCAL_GENERATED_SOURCES += $(GEN)

GEN := $(THIRD_PARTY)/skia/include/core/SkBitmap.h
$(GEN): $(SCRIPT)
$(GEN):
	perl $(SCRIPT) $@ "include/core/SkBitmap.h"
LOCAL_GENERATED_SOURCES += $(GEN)

GEN := $(THIRD_PARTY)/WebKit/WebKit/chromium/public/WebFormControlElement.h
$(GEN): $(SCRIPT)
$(GEN):
	perl $(SCRIPT) $@ "public/WebFormControlElement.h"
LOCAL_GENERATED_SOURCES += $(GEN)

GEN := $(THIRD_PARTY)/WebKit/WebKit/chromium/public/WebRegularExpression.h
$(GEN): $(SCRIPT)
$(GEN):
	perl $(SCRIPT) $@ "public/WebRegularExpression.h"
LOCAL_GENERATED_SOURCES += $(GEN)

GEN := $(THIRD_PARTY)/WebKit/WebKit/chromium/public/WebString.h
$(GEN): $(SCRIPT)
$(GEN):
	perl $(SCRIPT) $@ "public/WebString.h"
LOCAL_GENERATED_SOURCES += $(GEN)

GEN = $(INTERMEDIATES)/HaveAutofillInternal.h
$(GEN): $(CHECK_INTERNAL_HEADER_SCRIPT)
$(GEN):
	perl $(CHECK_INTERNAL_HEADER_SCRIPT) $@
LOCAL_GENERATED_SOURCES += $(GEN)

LOCAL_SRC_FILES += $(LOCAL_GENERATED_SOURCES)

include external/stlport/libstlport.mk

LOCAL_CFLAGS := -DHAVE_CONFIG_H -DANDROID -include "android/prefix.h" -fvisibility=hidden -DEXPAT_RELATIVE_PATH

include $(BUILD_STATIC_LIBRARY)
endif
