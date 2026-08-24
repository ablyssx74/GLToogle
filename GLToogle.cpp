/*
 * Copyright 2026, Kris Beazley GLToogle@epluribusunix.net
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Button.h>
#include <StatusBar.h>
#include <StringView.h>
#include <String.h>
#include <LayoutBuilder.h>
#include <Alert.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <thread>
#include <Notification.h>



namespace AppInfo {
	static const char* const APP_NAME = "GLToogle";
    static const char* const VERSION_STRING = "v1.0.0";

}

// =============================================================================
// Update Checker
// =============================================================================
static int32 BackgroundUpdateChecker(void* data) {
    snooze(5000000); 

    printf("[UpdateChecker] Asynchronous curl update checker running...\n");

    const char* targetUrl = "https://raw.githubusercontent.com/ablyssx74/GLtoogle/refs/heads/main/VERSION";

    BString shellCmdString;
    shellCmdString.SetToFormat("curl -sL \"%s\"", targetUrl);

    BString remoteVersionStr = "";
    
    FILE* pipeStream = popen(shellCmdString.String(), "r");
    if (pipeStream != nullptr) {
        char buffer[128] = {0};
        if (fgets(buffer, sizeof(buffer), pipeStream) != nullptr) {
            remoteVersionStr = buffer;
        }
        pclose(pipeStream);
    }

    remoteVersionStr.Trim(); 
     printf("[UpdateChecker] Raw text received from GitHub: '%s'\n", remoteVersionStr.String());

    if (remoteVersionStr.Length() > 0) {
    	BString currentAppStr = AppInfo::APP_NAME;
        BString currentVersionStr = AppInfo::VERSION_STRING;
        printf("[UpdateChecker] Local AppInfo text before cleaning: '%s'\n", currentVersionStr.String());

        int32 curMajor = 0, curMinor = 0, curRevision = 0;
        int32 remMajor = 0, remMinor = 0, remRevision = 0;


        if (sscanf(currentVersionStr.String(), "%*[^v]v%d.%d.%d", &curMajor, &curMinor, &curRevision) != 3) {
            sscanf(currentVersionStr.String(), "%*[^0-9]%d.%d.%d", &curMajor, &curMinor, &curRevision);
        }

        if (sscanf(remoteVersionStr.String(), "%*[^v]v%d.%d.%d", &remMajor, &remMinor, &remRevision) != 3) {
            sscanf(remoteVersionStr.String(), "%*[^0-9]%d.%d.%d", &remMajor, &remMinor, &remRevision);
        }

        
            printf("[UpdateChecker] Cleaned local target string: '%d.%d.%d'\n", curMajor, curMinor, curRevision);
        

        int32 currentFlattened = (curMajor * 10000) + (curMinor * 100) + curRevision;
        int32 remoteFlattened  = (remMajor * 10000) + (remMinor * 100) + remRevision;

      
            printf("[UpdateChecker] Calculated values for math match -> Local: %d | Remote: %d\n", 
                   (int)currentFlattened, (int)remoteFlattened);
        


			if (remoteFlattened > currentFlattened) {
			    printf("[UpdateChecker] Update found! Sending notification...\n");
			
			    BNotification updateAlert(B_INFORMATION_NOTIFICATION);
			    updateAlert.SetGroup(currentAppStr);
			    updateAlert.SetTitle("Update Available");
			    
			    BString alertContent;
			    // Added spaces around currentAppStr so it reads: "of GLToogle is available!"
			    alertContent << "A newer version of " << currentAppStr << " is available! (" << remoteVersionStr << ")";
			    updateAlert.SetContent(alertContent.String());
			    
			    updateAlert.Send();
			    printf("[UpdateChecker] Toast notification sent successfully.\n");
			} else {
			    printf("[UpdateChecker] Client binary is up to date.\n");
			}

		    } else {
		        printf("[UpdateChecker] CRITICAL ERR: Raw text data read from pipe buffer was empty!\n");
		    }
    
    return B_OK;
}



const char* kAppSignature = "application/x-vnd.gltoogle"; 

enum {
    MSG_UPDATE_PROGRESS = 'uprg',
    MSG_TASK_FINISHED   = 'tfsh',
    MSG_INSTALL_BASE    = 'inst',
    MSG_INSTALL_NEBULA  = 'ineb'
};

const char* kUrlGlvnd   = "https://github.com/X547/nvidia-haiku/releases/download/v0.0.1/libglvnd-1.7.0-4-x86_64.hpkg";
const char* kUrlNebula2 = "https://github.com/X547/nvidia-haiku/releases/download/v0.0.2/nebula-0.0.2-1.x86_64.hpkg";

class DriverToggleWindow : public BWindow {
public:
    DriverToggleWindow() : BWindow(BRect(0, 0, 540, 160), "Mesa / Nebula Stack Selector", 
                                   B_DOCUMENT_WINDOW, B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS) {
        
        BView* panel = new BView("main_panel", B_WILL_DRAW);
        panel->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

        fStatusLabel = new BStringView("status_label", "Checking driver state...");
        fStatusLabel->SetFont(be_bold_font);        
        fStatusLabel->SetAlignment(B_ALIGN_CENTER);
        fStatusBar = new BStatusBar("progress_bar", "Current Step:", "Ready");
        fStatusBar->SetMaxValue(100.0);
        fStatusBar->Hide();

        fActionBtn = new BButton("action_btn", "Evaluating State...", new BMessage(MSG_INSTALL_NEBULA));
        fStatusLabel->SetExplicitMinSize(BSize(540.0, B_SIZE_UNSET));

        BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
            .AddGroup(B_VERTICAL, B_USE_ITEM_SPACING)
                .SetInsets(B_USE_WINDOW_INSETS)
                .Add(fStatusLabel)
                .Add(fStatusBar)
                .AddGlue()
                .AddGroup(B_HORIZONTAL, B_USE_ITEM_SPACING)
                    .AddGlue()
                    .Add(fActionBtn) 
                    .AddGlue()
                .End()
            .End();

        // =========================================================================
        // AUTOMATED BACKGROUND UPDATE CHECKER THREAD INITIALIZATION
        // =========================================================================
        thread_id updateThread = spawn_thread(BackgroundUpdateChecker, "UpdateCheckerThread", B_NORMAL_PRIORITY, this);
        if (updateThread >= 0) {
            resume_thread(updateThread);
        }
        // =========================================================================

        CenterOnScreen();
        _UpdateDriverStatus();
    }


    ~DriverToggleWindow() {
        system("rm -f /tmp/libglvnd-1.7.0-4-x86_64.hpkg /tmp/nebula-0.0.2-1.x86_64.hpkg");
    }


    bool QuitRequested() override {
        be_app->PostMessage(B_QUIT_REQUESTED);
        return true;
    }

    void MessageReceived(BMessage* message) override {
        switch (message->what) {
            case MSG_UPDATE_PROGRESS: {
                float delta;
                const char* text;
                if (message->FindFloat("delta", &delta) == B_OK &&
                    message->FindString("text", &text) == B_OK) {
                    fStatusBar->Update(delta, text);
                }
                break;
            }

            case MSG_TASK_FINISHED: {
                const char* completionMsg = message->FindString("msg");
                
                fStatusBar->Update(100.0 - fStatusBar->CurrentValue(), NULL, completionMsg);
                snooze(500000); 
                fStatusBar->Hide();
                
                fActionBtn->SetEnabled(true);
                _UpdateDriverStatus();

                BNotification notification(B_INFORMATION_NOTIFICATION);
                notification.SetTitle("Driver Toggler");
                notification.SetContent(completionMsg);
                
                notification.Send();
                break;
            }


            case MSG_INSTALL_BASE: {
                _LockControls("Initiating Driver Uninstallation Sequence...");
                std::thread worker(&DriverToggleWindow::_WorkerBaseMesaStack, this);
                worker.detach();
                break;
            }

            case MSG_INSTALL_NEBULA: {
                _LockControls("Preparing Official Release Downloads...");
                std::thread worker(&DriverToggleWindow::_WorkerNebulaStack, this);
                worker.detach();
                break;
            }

            default:
                BWindow::MessageReceived(message);
                break;
        }
    }

private:
    void _LockControls(const char* initialLabel) {
        fStatusBar->Reset(initialLabel);
        fStatusBar->Show();
        fActionBtn->SetEnabled(false); 
    }


    void _PostProgressUpdate(float delta, const char* text) {
        BMessage msg(MSG_UPDATE_PROGRESS);
        msg.AddFloat("delta", delta);
        msg.AddString("text", text);
        PostMessage(&msg);
    }

    void _PostTaskFinished(const char* completionAlertText, bool isError = false) {
        BMessage msg(MSG_TASK_FINISHED);
        msg.AddString("msg", completionAlertText);
        msg.AddBool("is_error", isError); 
        PostMessage(&msg);
    }

    void _UpdateDriverStatus() {
        struct stat mesaBuffer;
        bool hasHardwareDriver = (stat("/boot/system/add-ons/opengl/egl_vendor.d/libEGL_mesa.so", &mesaBuffer) == 0);

        if (hasHardwareDriver) {
            fStatusLabel->SetText("Current State: NVIDIA Nebula Driver Stack Active");            
            fActionBtn->SetLabel("Restore Base Mesa Libs");
            fActionBtn->SetMessage(new BMessage(MSG_INSTALL_BASE));
            fActionBtn->SetEnabled(true);
        } else {
            fStatusLabel->SetText("Current State: Base Haiku Mesa Libs Active");            
            fActionBtn->SetLabel("Install NVIDIA Nebula Driver");
            fActionBtn->SetMessage(new BMessage(MSG_INSTALL_NEBULA));
            fActionBtn->SetEnabled(true);
        }
        
        InvalidateLayout();
    }

    void _WorkerBaseMesaStack() {
        _PostProgressUpdate(15.0, "Safely stripping NVIDIA Nebula modules...");
        system("pkgman uninstall -y nebula");
        
        _PostProgressUpdate(30.0, "Deploying baseline software fallback dependencies...");
        system("pkgman install -y mesa_swpipe mesa_lavapipe");
        
        _PostProgressUpdate(30.0, "Removing libglvnd infrastructure...");
        system("pkgman uninstall -y libglvnd");
        
        _PostProgressUpdate(20.0, "Finalizing baseline software restore...");
        _PostTaskFinished("Successfully reverted to Base Haiku Mesa Stack.");
    }


    bool _VerifyFileHash(const char* filePath, const char* expectedHash) {
        BString verifyCmd;
        verifyCmd.SetToFormat("echo \"%s  %s\" | sha256sum -c --status", expectedHash, filePath);
        
        int status = system(verifyCmd.String());
        return (status == 0);
    }

    void _WorkerNebulaStack() {
        system("mkdir -p /tmp/nebula_setup");

        const char* pathGlvnd = "/tmp/libglvnd-1.7.0-4-x86_64.hpkg";
        const char* pathNebula = "/tmp/nebula-0.0.2-1.x86_64.hpkg";

        const char* hashGlvnd = "0a4dba881e0a3c3f60a82cef84b7668a919569d8d880ee9011855b8539cb355b";
        const char* hashNebula = "2b9ba0fc817143502670073283f06d160e71aedeb101ab47616147641f51e763";

        struct stat glvndStat;
        bool glvndExistsAndValid = (stat(pathGlvnd, &glvndStat) == 0) && _VerifyFileHash(pathGlvnd, hashGlvnd);

        if (glvndExistsAndValid) {
            _PostProgressUpdate(10.0, "Found cached libglvnd framework package. Skipping download.");
            snooze(300000); 
        } else {
            unlink(pathGlvnd);
            _PostProgressUpdate(10.0, "Downloading official libglvnd framework package...");
            BString downloadGlvndCmd;
            downloadGlvndCmd.SetToFormat("curl -L --fail --retry 3 --connect-timeout 15 -o %s %s", pathGlvnd, kUrlGlvnd);
            
            if (system(downloadGlvndCmd.String()) != 0) {
                _PostTaskFinished("Error: libglvnd download failed. Check network link.", true);
                return;
            }
        }

        struct stat nebulaStat;
        bool nebulaExistsAndValid = (stat(pathNebula, &nebulaStat) == 0) && _VerifyFileHash(pathNebula, hashNebula);

        if (nebulaExistsAndValid) {
            _PostProgressUpdate(25.0, "Found cached NVIDIA Nebula kernel package. Skipping download.");
            snooze(300000);
        } else {
            unlink(pathNebula);
            _PostProgressUpdate(25.0, "Downloading official NVIDIA Nebula kernel package...");
            BString downloadNebulaCmd;
            downloadNebulaCmd.SetToFormat("curl -L --fail --retry 3 --connect-timeout 15 -o %s %s", pathNebula, kUrlNebula2);
            
            if (system(downloadNebulaCmd.String()) != 0) {
                _PostTaskFinished("Error: Nebula download failed. Check network link.", true);
                return;
            }	    
        }

        _PostProgressUpdate(15.0, "Verifying cryptographic signatures...");
        if (!_VerifyFileHash(pathGlvnd, hashGlvnd) || !_VerifyFileHash(pathNebula, hashNebula)) {
            system("rm -f /tmp/libglvnd-1.7.0-4-x86_64.hpkg /tmp/nebula-0.0.2-1.x86_64.hpkg");
            _PostTaskFinished("Error: Download verification failed. Package hash is corrupt.", true);
            return;
        }

        _PostProgressUpdate(15.0, "Deploying verified libglvnd framework package...");
        system("pkgman install -y /tmp/libglvnd-1.7.0-4-x86_64.hpkg");	

        _PostProgressUpdate(10.0, "Purging conflicting software Mesa frameworks...");
        system("pkgman uninstall -y mesa mesa_lavapipe");	    
        
        _PostProgressUpdate(10.0, "Deploying verified NVIDIA Nebula driver stack...");
        system("pkgman install -y /tmp/nebula-0.0.2-1.x86_64.hpkg");	
        
        _PostTaskFinished("Successfully initialized NVIDIA Nebula graphics drivers from official repository.");
    }



    BStringView*     fStatusLabel;
    BStatusBar*      fStatusBar;
    BButton*         fActionBtn; 
};

class DriverToggleApp : public BApplication {
public:
    DriverToggleApp() : BApplication(kAppSignature) {}
    void ReadyToRun() override {
        DriverToggleWindow* window = new DriverToggleWindow();
        window->Show();
    }
};

int main() {
    DriverToggleApp app;
    app.Run();
    return 0;
}
