package com.android.server.wifi;

import android.content.pm.UserInfo;
import android.net.wifi.WifiConfiguration;
import android.os.UserHandle;
import android.os.UserManager;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class ConfigurationMap {
    private final Map<Integer, WifiConfiguration> mPerID = new HashMap<>();
    private final Map<Integer, WifiConfiguration> mPerConfigKey = new HashMap<>();

    private final Map<Integer, WifiConfiguration> mPerIDForCurrentUser = new HashMap<>();
    private final Map<String, WifiConfiguration> mPerFQDNForCurrentUser = new HashMap<>();
    /**
     * List of all hidden networks in the current user's configuration.
     * Use this list as a param for directed scanning .
     */
    private final Set<Integer> mHiddenNetworkIdsForCurrentUser = new HashSet<>();

    private final UserManager mUserManager;

    private int mCurrentUserId = UserHandle.USER_SYSTEM;

    ConfigurationMap(UserManager userManager) {
        mUserManager = userManager;
    }

    // RW methods:
    public WifiConfiguration put(WifiConfiguration config) {
        final WifiConfiguration current = mPerID.put(config.networkId, config);
        mPerConfigKey.put(config.configKey().hashCode(), config);   // This is ridiculous...
        if (WifiConfigurationUtil.isVisibleToAnyProfile(config,
                mUserManager.getProfiles(mCurrentUserId))) {
            mPerIDForCurrentUser.put(config.networkId, config);
            if (config.FQDN != null && config.FQDN.length() > 0) {
                mPerFQDNForCurrentUser.put(config.FQDN, config);
            }
            if (config.hiddenSSID) {
                mHiddenNetworkIdsForCurrentUser.add(config.networkId);
            }
        }
        return current;
    }

    public WifiConfiguration remove(int netID) {
        WifiConfiguration config = mPerID.remove(netID);
        if (config == null) {
            return null;
        }
        mPerConfigKey.remove(config.configKey().hashCode());

        mPerIDForCurrentUser.remove(netID);
        Iterator<Map.Entry<String, WifiConfiguration>> entries =
                mPerFQDNForCurrentUser.entrySet().iterator();
        while (entries.hasNext()) {
            if (entries.next().getValue().networkId == netID) {
                entries.remove();
                break;
            }
        }
        mHiddenNetworkIdsForCurrentUser.remove(netID);
        return config;
    }

    public void clear() {
        mPerID.clear();
        mPerConfigKey.clear();
        mPerIDForCurrentUser.clear();
        mPerFQDNForCurrentUser.clear();
        mHiddenNetworkIdsForCurrentUser.clear();
    }

    /**
     * Handles the switch to a different foreground user:
     * - Hides private network configurations belonging to the previous foreground user
     * - Reveals private network configurations belonging to the new foreground user
     *
     * @param userId the id of the new foreground user
     * @return a list of {@link WifiConfiguration}s that became hidden because of the user switch
     */
    public List<WifiConfiguration> handleUserSwitch(int userId) {
        mPerIDForCurrentUser.clear();
        mPerFQDNForCurrentUser.clear();
        mHiddenNetworkIdsForCurrentUser.clear();

        final List<UserInfo> previousUserProfiles = mUserManager.getProfiles(mCurrentUserId);
        mCurrentUserId = userId;
        final List<UserInfo> currentUserProfiles = mUserManager.getProfiles(mCurrentUserId);

        final List<WifiConfiguration> hiddenConfigurations = new ArrayList<>();
        for (Map.Entry<Integer, WifiConfiguration> entry : mPerID.entrySet()) {
            final WifiConfiguration config = entry.getValue();
            if (WifiConfigurationUtil.isVisibleToAnyProfile(config, currentUserProfiles)) {
                mPerIDForCurrentUser.put(entry.getKey(), config);
                if (config.FQDN != null && config.FQDN.length() > 0) {
                    mPerFQDNForCurrentUser.put(config.FQDN, config);
                }
                if (config.hiddenSSID) {
                    mHiddenNetworkIdsForCurrentUser.add(config.networkId);
                }
            } else if (WifiConfigurationUtil.isVisibleToAnyProfile(config, previousUserProfiles)) {
                hiddenConfigurations.add(config);
            }
        }

        return hiddenConfigurations;
    }

    // RO methods:
    public WifiConfiguration getForAllUsers(int netid) {
        return mPerID.get(netid);
    }

    public WifiConfiguration getForCurrentUser(int netid) {
        return mPerIDForCurrentUser.get(netid);
    }

    public int sizeForAllUsers() {
        return mPerID.size();
    }

    public int sizeForCurrentUser() {
        return mPerIDForCurrentUser.size();
    }

    public WifiConfiguration getByFQDNForCurrentUser(String fqdn) {
        return mPerFQDNForCurrentUser.get(fqdn);
    }

    public WifiConfiguration getByConfigKeyForCurrentUser(String key) {
        if (key == null) {
            return null;
        }
        for (WifiConfiguration config : mPerIDForCurrentUser.values()) {
            if (config.configKey().equals(key)) {
                return config;
            }
        }
        return null;
    }

    public WifiConfiguration getByConfigKeyIDForAllUsers(int id) {
        return mPerConfigKey.get(id);
    }

    public Collection<WifiConfiguration> getEnabledNetworksForCurrentUser() {
        List<WifiConfiguration> list = new ArrayList<>();
        for (WifiConfiguration config : mPerIDForCurrentUser.values()) {
            if (config.status != WifiConfiguration.Status.DISABLED) {
                list.add(config);
            }
        }
        return list;
    }

    public WifiConfiguration getEphemeralForCurrentUser(String ssid) {
        for (WifiConfiguration config : mPerIDForCurrentUser.values()) {
            if (ssid.equals(config.SSID) && config.ephemeral) {
                return config;
            }
        }
        return null;
    }

    public Collection<WifiConfiguration> valuesForAllUsers() {
        return mPerID.values();
    }

    public Collection<WifiConfiguration> valuesForCurrentUser() {
        return mPerIDForCurrentUser.values();
    }

    public Set<Integer> getHiddenNetworkIdsForCurrentUser() {
        return mHiddenNetworkIdsForCurrentUser;
    }
}
