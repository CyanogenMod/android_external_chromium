// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/pref_value_store.h"

<<<<<<< HEAD
#ifndef ANDROID
#include "chrome/browser/browser_thread.h"
#include "chrome/browser/extensions/extension_pref_store.h"
#include "chrome/browser/policy/configuration_policy_pref_store.h"
#include "chrome/browser/prefs/command_line_pref_store.h"
#endif
#include "chrome/browser/prefs/default_pref_store.h"
#ifndef ANDROID
#include "chrome/common/json_pref_store.h"
#include "chrome/common/notification_service.h"
#endif
=======
#include "chrome/browser/prefs/pref_notifier.h"
>>>>>>> chromium.org at r10.0.621.0

PrefValueStore::PrefStoreKeeper::PrefStoreKeeper()
    : pref_value_store_(NULL),
      type_(PrefValueStore::INVALID_STORE) {
}

PrefValueStore::PrefStoreKeeper::~PrefStoreKeeper() {
  if (pref_store_.get())
    pref_store_->RemoveObserver(this);
}

void PrefValueStore::PrefStoreKeeper::Initialize(
    PrefValueStore* store,
    PrefStore* pref_store,
    PrefValueStore::PrefStoreType type) {
  if (pref_store_.get())
    pref_store_->RemoveObserver(this);
  type_ = type;
  pref_value_store_ = store;
  pref_store_.reset(pref_store);
  if (pref_store_.get())
    pref_store_->AddObserver(this);
}

<<<<<<< HEAD
}  // namespace

// static
PrefValueStore* PrefValueStore::CreatePrefValueStore(
    const FilePath& pref_filename,
    Profile* profile,
    bool user_only) {
#ifdef ANDROID
  return new PrefValueStore(NULL, NULL, NULL, NULL, NULL, NULL, new DefaultPrefStore(), profile);
#else
  using policy::ConfigurationPolicyPrefStore;
  ConfigurationPolicyPrefStore* managed = NULL;
  ConfigurationPolicyPrefStore* device_management = NULL;
  ExtensionPrefStore* extension = NULL;
  CommandLinePrefStore* command_line = NULL;
  ConfigurationPolicyPrefStore* recommended = NULL;

  JsonPrefStore* user = new JsonPrefStore(
      pref_filename,
      BrowserThread::GetMessageLoopProxyForThread(BrowserThread::FILE));
  DefaultPrefStore* default_store = new DefaultPrefStore();

  if (!user_only) {
    managed =
        ConfigurationPolicyPrefStore::CreateManagedPlatformPolicyPrefStore();
    device_management =
        ConfigurationPolicyPrefStore::CreateDeviceManagementPolicyPrefStore(
            profile);
    extension = new ExtensionPrefStore(profile, PrefNotifier::EXTENSION_STORE);
    command_line = new CommandLinePrefStore(CommandLine::ForCurrentProcess());
    recommended =
        ConfigurationPolicyPrefStore::CreateRecommendedPolicyPrefStore();
  }

  return new PrefValueStore(managed, device_management, extension,
                            command_line, user, recommended, default_store,
                            profile);
#endif
=======
void PrefValueStore::PrefStoreKeeper::OnPrefValueChanged(
    const std::string& key) {
  pref_value_store_->OnPrefValueChanged(type_, key);
}

void PrefValueStore::PrefStoreKeeper::OnInitializationCompleted() {
  pref_value_store_->OnInitializationCompleted(type_);
}

PrefValueStore::PrefValueStore(PrefStore* managed_platform_prefs,
                               PrefStore* device_management_prefs,
                               PrefStore* extension_prefs,
                               PrefStore* command_line_prefs,
                               PrefStore* user_prefs,
                               PrefStore* recommended_prefs,
                               PrefStore* default_prefs,
                               PrefNotifier* pref_notifier)
    : pref_notifier_(pref_notifier) {
  InitPrefStore(MANAGED_PLATFORM_STORE, managed_platform_prefs);
  InitPrefStore(DEVICE_MANAGEMENT_STORE, device_management_prefs);
  InitPrefStore(EXTENSION_STORE, extension_prefs);
  InitPrefStore(COMMAND_LINE_STORE, command_line_prefs);
  InitPrefStore(USER_STORE, user_prefs);
  InitPrefStore(RECOMMENDED_STORE, recommended_prefs);
  InitPrefStore(DEFAULT_STORE, default_prefs);

  CheckInitializationCompleted();
>>>>>>> chromium.org at r10.0.621.0
}

PrefValueStore::~PrefValueStore() {}

bool PrefValueStore::GetValue(const std::string& name,
                              Value** out_value) const {
  // Check the |PrefStore|s in order of their priority from highest to lowest
  // to find the value of the preference described by the given preference name.
  for (size_t i = 0; i <= PREF_STORE_TYPE_MAX; ++i) {
    if (GetValueFromStore(name.c_str(), static_cast<PrefStoreType>(i),
                          out_value))
      return true;
  }
  return false;
}

void PrefValueStore::RegisterPreferenceType(const std::string& name,
                                            Value::ValueType type) {
  pref_types_[name] = type;
}

Value::ValueType PrefValueStore::GetRegisteredType(
    const std::string& name) const {
  PrefTypeMap::const_iterator found = pref_types_.find(name);
  if (found == pref_types_.end())
    return Value::TYPE_NULL;
  return found->second;
}

bool PrefValueStore::HasPrefPath(const char* path) const {
  Value* tmp_value = NULL;
  const std::string name(path);
  bool rv = GetValue(name, &tmp_value);
  // Merely registering a pref doesn't count as "having" it: we require a
  // non-default value set.
  return rv && !PrefValueFromDefaultStore(path);
}

void PrefValueStore::NotifyPrefChanged(
    const char* path,
    PrefValueStore::PrefStoreType new_store) {
  DCHECK(new_store != INVALID_STORE);

  // If this pref is not registered, just discard the notification.
  if (!pref_types_.count(path))
    return;

  bool changed = true;
  // Replying that the pref has changed in case the new store is invalid may
  // cause problems, but it's the safer choice.
  if (new_store != INVALID_STORE) {
    PrefStoreType controller = ControllingPrefStoreForPref(path);
    DCHECK(controller != INVALID_STORE);
    // If the pref is controlled by a higher-priority store, its effective value
    // cannot have changed.
    if (controller != INVALID_STORE &&
        controller < new_store) {
      changed = false;
    }
  }

  if (changed)
    pref_notifier_->OnPreferenceChanged(path);
}

bool PrefValueStore::PrefValueInManagedPlatformStore(const char* name) const {
  return PrefValueInStore(name, MANAGED_PLATFORM_STORE);
}

bool PrefValueStore::PrefValueInDeviceManagementStore(const char* name) const {
  return PrefValueInStore(name, DEVICE_MANAGEMENT_STORE);
}

bool PrefValueStore::PrefValueInExtensionStore(const char* name) const {
  return PrefValueInStore(name, EXTENSION_STORE);
}

bool PrefValueStore::PrefValueInUserStore(const char* name) const {
  return PrefValueInStore(name, USER_STORE);
}

bool PrefValueStore::PrefValueFromExtensionStore(const char* name) const {
  return ControllingPrefStoreForPref(name) == EXTENSION_STORE;
}

bool PrefValueStore::PrefValueFromUserStore(const char* name) const {
  return ControllingPrefStoreForPref(name) == USER_STORE;
}

bool PrefValueStore::PrefValueFromDefaultStore(const char* name) const {
  return ControllingPrefStoreForPref(name) == DEFAULT_STORE;
}

bool PrefValueStore::PrefValueUserModifiable(const char* name) const {
  PrefStoreType effective_store = ControllingPrefStoreForPref(name);
  return effective_store >= USER_STORE ||
         effective_store == INVALID_STORE;
}

// Returns true if the actual value is a valid type for the expected type when
// found in the given store.
bool PrefValueStore::IsValidType(Value::ValueType expected,
                                 Value::ValueType actual,
                                 PrefValueStore::PrefStoreType store) {
  if (expected == actual)
    return true;

  // Dictionaries and lists are allowed to hold TYPE_NULL values too, but only
  // in the default pref store.
  if (store == DEFAULT_STORE &&
      actual == Value::TYPE_NULL &&
      (expected == Value::TYPE_DICTIONARY || expected == Value::TYPE_LIST)) {
    return true;
  }
  return false;
}

bool PrefValueStore::PrefValueInStore(
    const char* name,
    PrefValueStore::PrefStoreType store) const {
  // Declare a temp Value* and call GetValueFromStore,
  // ignoring the output value.
  Value* tmp_value = NULL;
  return GetValueFromStore(name, store, &tmp_value);
}

bool PrefValueStore::PrefValueInStoreRange(
    const char* name,
    PrefValueStore::PrefStoreType first_checked_store,
    PrefValueStore::PrefStoreType last_checked_store) const {
  if (first_checked_store > last_checked_store) {
    NOTREACHED();
    return false;
  }

  for (size_t i = first_checked_store;
       i <= static_cast<size_t>(last_checked_store); ++i) {
    if (PrefValueInStore(name, static_cast<PrefStoreType>(i)))
      return true;
  }
  return false;
}

<<<<<<< HEAD
#ifndef ANDROID
void PrefValueStore::RefreshPolicyPrefsOnFileThread(
    BrowserThread::ID calling_thread_id,
    PrefStore* new_managed_platform_pref_store,
    PrefStore* new_device_management_pref_store,
    PrefStore* new_recommended_pref_store,
    AfterRefreshCallback* callback_pointer) {
  scoped_ptr<AfterRefreshCallback> callback(callback_pointer);
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  scoped_ptr<PrefStore> managed_platform_pref_store(
      new_managed_platform_pref_store);
  scoped_ptr<PrefStore> device_management_pref_store(
      new_device_management_pref_store);
  scoped_ptr<PrefStore> recommended_pref_store(new_recommended_pref_store);

  PrefStore::PrefReadError read_error =
      new_managed_platform_pref_store->ReadPrefs();
  if (read_error != PrefStore::PREF_READ_ERROR_NONE) {
    LOG(ERROR) << "refresh of managed policy failed: PrefReadError = "
               << read_error;
    return;
  }

  read_error = new_device_management_pref_store->ReadPrefs();
  if (read_error != PrefStore::PREF_READ_ERROR_NONE) {
    LOG(ERROR) << "refresh of device management policy failed: "
               << "PrefReadError = " << read_error;
    return;
=======
PrefValueStore::PrefStoreType PrefValueStore::ControllingPrefStoreForPref(
    const char* name) const {
  for (size_t i = 0; i <= PREF_STORE_TYPE_MAX; ++i) {
    if (PrefValueInStore(name, static_cast<PrefStoreType>(i)))
      return static_cast<PrefStoreType>(i);
>>>>>>> chromium.org at r10.0.621.0
  }
  return INVALID_STORE;
}

bool PrefValueStore::GetValueFromStore(const char* name,
                                       PrefValueStore::PrefStoreType store_type,
                                       Value** out_value) const {
  // Only return true if we find a value and it is the correct type, so stale
  // values with the incorrect type will be ignored.
  const PrefStore* store = GetPrefStore(static_cast<PrefStoreType>(store_type));
  if (store) {
    switch (store->GetValue(name, out_value)) {
      case PrefStore::READ_USE_DEFAULT:
        store = GetPrefStore(DEFAULT_STORE);
        if (!store || store->GetValue(name, out_value) != PrefStore::READ_OK) {
          *out_value = NULL;
          return false;
        }
        // Fall through...
      case PrefStore::READ_OK:
        if (IsValidType(GetRegisteredType(name),
                        (*out_value)->GetType(),
                        store_type)) {
          return true;
        }
        break;
      case PrefStore::READ_NO_VALUE:
        break;
    }
  }

  // No valid value found for the given preference name: set the return false.
  *out_value = NULL;
  return false;
}

void PrefValueStore::OnPrefValueChanged(PrefValueStore::PrefStoreType type,
                                        const std::string& key) {
  NotifyPrefChanged(key.c_str(), type);
}

void PrefValueStore::OnInitializationCompleted(
    PrefValueStore::PrefStoreType type) {
  CheckInitializationCompleted();
}
#endif // ANDROID

<<<<<<< HEAD
bool PrefValueStore::HasPolicyConflictingUserProxySettings() {
#if !defined(ANDROID)
  using policy::ConfigurationPolicyPrefStore;
  ConfigurationPolicyPrefStore::ProxyPreferenceSet proxy_prefs;
  ConfigurationPolicyPrefStore::GetProxyPreferenceSet(&proxy_prefs);
  ConfigurationPolicyPrefStore::ProxyPreferenceSet::const_iterator i;
  for (i = proxy_prefs.begin(); i != proxy_prefs.end(); ++i) {
    if ((PrefValueInManagedPlatformStore(*i) ||
         PrefValueInDeviceManagementStore(*i)) &&
        PrefValueInStoreRange(*i,
                              PrefNotifier::COMMAND_LINE_STORE,
                              PrefNotifier::USER_STORE))
      return true;
  }
#endif
  return false;
=======
void PrefValueStore::InitPrefStore(PrefValueStore::PrefStoreType type,
                                   PrefStore* pref_store) {
  pref_stores_[type].Initialize(this, pref_store, type);
>>>>>>> chromium.org at r10.0.621.0
}

void PrefValueStore::CheckInitializationCompleted() {
  for (size_t i = 0; i <= PREF_STORE_TYPE_MAX; ++i) {
    PrefStore* store = GetPrefStore(static_cast<PrefStoreType>(i));
    if (store && !store->IsInitializationComplete())
      return;
  }
  pref_notifier_->OnInitializationCompleted();
}
