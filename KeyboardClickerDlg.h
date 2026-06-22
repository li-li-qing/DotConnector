#pragma once

#include <array>
#include <atomic>
#include <random>
#include <thread>
#include <vector>

#include <afxcmn.h>
#include <afxwin.h>
#include <afxdialogex.h>

#include "KeyCaptureEdit.h"

class CKeyboardClickerDlg : public CDialogEx
{
public:
    CKeyboardClickerDlg();

protected:
    BOOL OnInitDialog() override;
    void DoDataExchange(CDataExchange* pDX) override;

    afx_msg void OnDestroy();
    afx_msg void OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnStart();
    afx_msg void OnStop();
    afx_msg void OnAdvancedFeatures();
    // 切换高级技能自动检测的开始/结束状态。
    afx_msg void OnToggleSkillMonitoring();
    afx_msg void OnClearDebugLog();
    afx_msg void OnHornEnabled();
    afx_msg void OnDebuffOptions();
    afx_msg void OnDebuffOptionChanged();
    afx_msg void OnAddHornSample();
    afx_msg void OnTestHornRecognition();
    afx_msg void OnSelectSkillRegion(UINT nID);
    afx_msg void OnAddSkillSample(UINT nID);
    afx_msg void OnDetectSkill(UINT nID);
    afx_msg void OnSkillEnabledChanged(UINT nID);
    afx_msg void OnSkillIntervalChanged(UINT nID);
    afx_msg void OnSkillDebuffChanged(UINT nID);
    afx_msg void OnAddDebuffSample(UINT nID);
    afx_msg void OnSelectDebuffRegion();
    afx_msg void OnSelectHornRegion();
    afx_msg void OnPageTabChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnKeyCaptured(WPARAM wParam, LPARAM lParam);
    // 接收后台线程投递的状态文本。
    afx_msg LRESULT OnAsyncStatus(WPARAM wParam, LPARAM lParam);
    // 处理后台识图任务完成后的 UI 更新和按键发送。
    afx_msg LRESULT OnSkillWorkerFinished(WPARAM wParam, LPARAM lParam);
    // 处理后台喊话任务完成后的热键恢复和状态刷新。
    afx_msg LRESULT OnShoutWorkerFinished(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

private:
    struct KeyJob
    {
        UINT vk = 0;
        UINT intervalMs = 0;
        bool enabled = false;
    };

    struct ClickActionControls
    {
        CStatic indexLabel;
        CKeyCaptureEdit keyEdit;
        CEdit intervalEdit;
    };

    struct ShoutActionControls
    {
        CStatic indexLabel;
        CKeyCaptureEdit triggerKeyEdit;
        CEdit triggerCountEdit;
        CEdit chatDelayEdit;
        CEdit messageEdit;
        CEdit skillWaitEdit;
        CEdit sequenceEdit;
        CKeyCaptureEdit cooldownSkillKeyEdit;
    };

    struct ShoutActionState
    {
        bool hotkeyRegistered = false;
        UINT pressCount = 0;
        ULONGLONG lastPressTick = 0;
        ULONGLONG lastMessageTick = 0;
    };

    struct HornState
    {
        bool enabled = false;
        UINT intervalMs = 500;
        double similarity = 0.9;
        bool hasDebuffRegion = false;
        CRect debuffRegion;
        std::array<bool, 4> debuffs = {};
        bool debuffPanelVisible = false;
        bool lastDebuffFound = false;
        bool lastHornAvailable = false;
        ULONGLONG lastHornDebugTick = 0;
        ULONGLONG lastHornPressStatusTick = 0;
        bool actionLockedUntilDebuffClears = false;
        CString cachedDebuffPictures;
        ULONGLONG lastPictureRefreshTick = 0;
        bool pictureCacheValid = false;
    };

    struct AdvancedSkillState
    {
        bool enabled = true;
        UINT skillVk = 0;
        UINT priority = 1;
        // 该技能参与 Debuff 扫描的最小间隔，越低反应越快但识图压力越大。
        UINT detectIntervalMs = 50;
        // 技能释放后的防重复触发时间，避免同一控制状态下连续乱按。
        UINT releaseIntervalMs = 500;
        // 上一次提交该技能自动检测任务的时间。
        ULONGLONG lastDetectTick = 0;
        // 上一次自动释放该技能的时间。
        ULONGLONG lastReleaseTick = 0;
        bool hasRegion = false;
        CRect region;
        std::array<bool, 4> debuffs = {};
        CString cachedPictures;
        // 缓存该技能勾选 Debuff 对应的图片列表，避免自动检测时反复扫描磁盘。
        CString cachedDebuffPictures;
        // 记录后台空闲探测得到的技能图标可用状态，自动解除时可直接走快速释放路径。
        bool lastSkillAvailable = false;
        // 记录技能图标可用状态的采样时间，超过有效期后只作为弱参考。
        ULONGLONG lastSkillAvailableTick = 0;
    };

    struct AdvancedSkillControls
    {
        CStatic group;
        CButton enabledCheck;
        CStatic keyLabel;
        CKeyCaptureEdit keyEdit;
        CStatic priorityLabel;
        CEdit priorityEdit;
        CStatic detectIntervalLabel;
        CEdit detectIntervalEdit;
        CStatic intervalLabel;
        CEdit intervalEdit;
        CStatic timingHelpLabel;
        CButton regionButton;
        CButton sampleButton;
        CButton detectButton;
        CStatic regionLabel;
        std::array<CButton, 4> debuffChecks;
    };

    static constexpr int HotkeyCaptureTarget = 1;
    static constexpr int HotkeyShoutBase = 10;
    static constexpr size_t MaxClickActions = 50;
    static constexpr size_t MaxShoutActions = 50;
    static constexpr size_t AdvancedSkillCount = 4;
    static constexpr ULONGLONG ShoutSequenceMs = 1000;
    static constexpr ULONGLONG ShoutCooldownMs = 5000;
    static constexpr UINT_PTR TimerBase = 2000;

    void CaptureTargetUnderCursor();
    void StopJobs();
    void LoadSettings();
    void SaveSettings() const;
    bool ReadJobs();
    bool SendKey(UINT vk);
    bool SendShoutMessage(size_t index);
    // 快照喊话配置并投递到后台线程执行。
    bool QueueShoutMessage(size_t index, UINT startDelayMs);
    void HandleShoutHotkey(size_t index);
    void TriggerCooldownSkill(size_t index);
    void OnAddClickAction();
    void OnRemoveClickAction();
    void OnAddShoutAction();
    void OnRemoveShoutAction();
    void InitializeClickActionControls();
    void ConfigureDefaultClickActions();
    void ShowClickActionRows();
    void LayoutControls();
    // 布局基础自动按键与喊话页面。
    void LayoutBasePage();
    // 布局高级识图技能页面。
    void LayoutAdvancedPage();
    // 布局底部状态栏与调试输出区域。
    void LayoutDebugPanel();
    void MoveDlgItem(int controlId, int x, int y, int width, int height);
    size_t VisibleClickRowsPerColumn() const;
    size_t VisibleClickRowCapacity() const;
    size_t VisibleShoutRowCapacity() const;
    void ClampScrollOffsets();
    void UpdateScrollBars();
    void InitializeShoutActionControls();
    void ConfigureDefaultShoutActions();
    void InitializeAdvancedControls();
    void ShowShoutActionRows();
    void ShowBasePageControls(int show);
    void ShowAdvancedPageControls(int show);
    void UpdateAdvancedControls();
    // 根据运行状态刷新“开始/结束”按钮。
    void UpdateSkillMonitorButtonText();
    void UpdateDebuffSummary();
    void RegisterAllShoutHotkeys();
    void RegisterShoutHotkey(size_t index);
    void UnregisterAllShoutHotkeys();
    void UnregisterShoutHotkey(size_t index);
    void SuspendShoutHotkeysForKey(UINT vk, std::array<bool, MaxShoutActions>& suspended);
    void RestoreSuspendedShoutHotkeys(const std::array<bool, MaxShoutActions>& suspended);
    bool PrepareForegroundTarget();
    bool SendInputKey(UINT vk);
    bool SendInputShortcut(UINT modifierVk, UINT vk);
    bool SendMouseWheel(int delta);
    bool RegisterDmPlugin();
    bool RequestAdvancedPassword();
    void MarkAdvancedAuthorized();
    void ReleaseDmResources();
    bool InitializeDmResources(IDispatch* dm);
    bool SelectScreenRegion(CRect& region);
    void SetRegionStatus(CStatic& label, bool hasRegion, const CRect& region, LPCTSTR emptyText);
    void StopHornMonitoring();
    // 回收已经结束的后台线程资源。
    void JoinFinishedAsyncWorkers();
    // 从后台线程安全投递状态文本到主窗口。
    void PostStatusFromWorker(const CString& text);
    bool HasEnabledAdvancedSkill() const;
    void UpdateSkillMonitoringTimer();
    UINT ReadSkillDetectIntervalMs(size_t index) const;
    UINT ReadSkillReleaseIntervalMs(size_t index) const;
    UINT NextSkillMonitorDelay() const;
    bool DetectSkillOnce(size_t index);
    bool DetectBestSkillOnce(bool saveSettings = true, bool respectIntervals = false);
    // 快照识图配置并投递到后台线程执行。
    bool LaunchSkillDetection(const std::vector<size_t>& order, bool saveSettings, bool respectIntervals);
    bool AnyConfiguredDebuffFound();
    double ReadHornSimilarity() const;
    void InvalidateHornPictureCache();
    void RefreshHornPictureCache(bool force);
    bool ValidateAdvancedSkillSettings(size_t index, CString& skillPictures, CString& selectedPictures);
    bool ValidateSkillIconSettings(size_t index, CString& skillPictures);
    CString BuildSkillPictureList(size_t index) const;
    CString BuildSkillDebuffPictureList(size_t index) const;
    CString BuildSelectedDebuffPictureList() const;
    bool AppendPictureIfExists(CString& pictures, const CString& relativePath) const;
    void AppendPicturesFromDirectory(CString& pictures, LPCTSTR relativeDirectory, LPCTSTR filePattern) const;
    CString FilterExistingPictures(const CString& pictures) const;
    CString NormalizeDmPicturePath(const CString& path) const;
    CString GetHornSampleDirectory() const;
    CString GetSkillSampleDirectory(size_t index) const;
    CString GetDebuffSampleDirectory(size_t index) const;
    bool AddPictureSamples(const CString& directory, LPCTSTR label);
    bool AddDebuffSample(size_t index);
    CString MakeUniqueSamplePath(const CString& directory, const CString& extension) const;
    CString GetDmResourceRootForUse() const;
    bool DmFindPicEx(const CRect& region, const CString& pictures, CString& result);
    bool TestHornRecognitionOnce();
    bool ValidateHornRecognitionInputs(CString& hornPictures);
    void LogFindPicResult(LPCTSTR label, const CRect& region, const CString& pictures, const CString& result, bool found);
    CString FormatDmRegResult(long result) const;
    void AppendDebugText(const CString& text);
    bool ExecuteShoutSequence(size_t index);
    bool ExecuteReportSequenceToken(const CString& token);
    bool ReadClipboardText(CString& text) const;
    bool SetClipboardText(const CString& text) const;
    bool SetAndVerifyClipboardText(const CString& text) const;
    void ClearClipboard() const;
    void ScheduleNext(size_t index);
    void SetStatus(const CString& text);
    CString FormatHwnd(HWND hwnd) const;
    CString ReadWindowText(HWND hwnd) const;
    bool TryParseVirtualKey(const CString& token, UINT& vk) const;
    bool ReadUInt(int controlId, UINT& value) const;
    bool ReadUIntFromWindow(const CWnd& window, UINT& value) const;
    CString GetWindowTextString(const CWnd& window) const;
    UINT NextDelay(UINT intervalMs);
    bool IsExtendedKey(UINT vk) const;

    HWND m_targetHwnd = nullptr;
    bool m_running = false;
    size_t m_clickActionCount = 0;
    size_t m_shoutActionCount = 0;
    size_t m_clickScrollOffset = 0;
    size_t m_shoutScrollOffset = 0;
    UINT m_randomDeviationMs = 0;
    std::mt19937 m_rng;
    CScrollBar m_clickScrollBar;
    CScrollBar m_shoutScrollBar;
    std::array<KeyJob, MaxClickActions> m_jobs = {};
    std::array<ClickActionControls, MaxClickActions> m_clickControls;
    std::array<ShoutActionControls, MaxShoutActions> m_shoutControls;
    std::array<ShoutActionState, MaxShoutActions> m_shoutStates;
    HMODULE m_dmModule = nullptr;
    IDispatch* m_dmObject = nullptr;
    bool m_advancedInitialized = false;
    bool m_advancedAuthorized = false;
    int m_activePage = 0;
    CTabCtrl m_pageTabs;
    CStatic m_advancedHintLabel;
    CStatic m_hornGroup;
    CButton m_hornEnabledCheck;
    CStatic m_hornIntervalLabel;
    CEdit m_hornIntervalEdit;
    CStatic m_hornSimilarityLabel;
    CEdit m_hornSimilarityEdit;
    CStatic m_hornSkillLabel;
    CKeyCaptureEdit m_hornSkillEdit;
    CButton m_debuffRegionButton;
    CButton m_hornRegionButton;
    CButton m_hornSampleButton;
    CButton m_hornTestButton;
    CButton m_debuffOptionsButton;
    CStatic m_debuffRegionLabel;
    CStatic m_hornRegionLabel;
    CStatic m_debuffSummaryLabel;
    std::array<CButton, 4> m_debuffChecks;
    std::array<CButton, 4> m_debuffSampleButtons;
    std::array<AdvancedSkillControls, AdvancedSkillCount> m_skillControls;
    std::array<AdvancedSkillState, AdvancedSkillCount> m_skillStates;
    HornState m_hornState;
    // 高级页自动检测的开始/结束按钮。
    CButton m_skillMonitorButton;
    // 后台识图线程，避免阻塞 UI 消息循环。
    std::thread m_skillWorker;
    // 后台喊话线程，承载 Sleep 和剪贴板流程。
    std::thread m_shoutWorker;
    // 标记识图后台任务是否正在运行。
    std::atomic_bool m_skillWorkerActive = false;
    // 标记喊话后台任务是否正在运行。
    std::atomic_bool m_shoutWorkerActive = false;
    // 空闲时轮询探测技能图标可用状态的起始下标，避免每轮扫描所有技能。
    size_t m_nextAvailabilityProbeIndex = 0;
    // 标记后台已经快速发送了解控技能，防止结果回传前重复触发。
    std::atomic_bool m_skillInputPending = false;
    // 标记窗口正在关闭，阻止后台线程继续投递消息。
    std::atomic_bool m_closing = false;
    CString m_lastStatusText;
};
