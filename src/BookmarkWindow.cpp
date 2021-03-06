#include <View.h>
#include <String.h>
#include <Alert.h>
#include <Messenger.h>
#include <File.h>
#include <Path.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ConfigFile.h"
#include "BookmarkWindow.h"
#include "FTPWindow.h"
#include "FtpPositive.h"

enum {
	CONNECT_CLICKED = 'svcn',
	SAVE_CLICKED = 'save',
	EDIT_CHANGED = 'edit',
};

TBookmarkWindow::TBookmarkWindow(float x, float y, const char *title, const char *dir)
	:	BWindow(BRect(x, y, x + 290, y + 250), title, B_FLOATING_WINDOW_LOOK,
				B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fBookmarkDir.SetTo(dir);
	fNewBookmark = true;
	
	BView *bgview = new BView(Bounds(), "bgview", B_FOLLOW_NONE, 0);
	bgview->SetViewColor(217, 217, 217);
	AddChild(bgview);
	
	float lx = 10, rx = Bounds().right - 10, div = 70;
	
	fBookmarkName = new TOAEdit(BRect(lx, 20, rx, 38), "BookmarkName", "Name:", "New Bookmark", new BMessage(EDIT_CHANGED));
	fHostAddress = new TOAEdit(BRect(lx, 40, rx, 58), "HostAddress", "Address:", "", new BMessage(EDIT_CHANGED));
	fPort = new TOAEdit(BRect(lx, 60, rx, 78), "Port", "Port:", "21", new BMessage(EDIT_CHANGED));
	fUsername = new TOAEdit(BRect(lx, 80, rx, 98), "Username", "User Name:", "", new BMessage(EDIT_CHANGED));
	fPassword = new TOAEdit(BRect(lx, 100, rx, 118), "Password", "Password:", "", new BMessage(EDIT_CHANGED));
	fEncoder = new TOAEdit(BRect(lx, 120, rx, 138), "Encoder", "Encoder:", "", NULL);
	fRemotePath = new TOAEdit(BRect(lx, 140, rx, 158), "RemotePath", "Remote Path:", "", NULL);
	fLocalPath = new TOAEdit(BRect(lx, 160, rx, 178), "LocalPath", "Local Path:", TFtpPositive::GetDefaultLocalDir().String(), NULL);
	
	fBookmarkName->SetDivider(div);
	fHostAddress->SetDivider(div);
	fPort->SetDivider(div);
	fUsername->SetDivider(div);
	fPassword->SetDivider(div);
	fEncoder->SetDivider(div);
	fRemotePath->SetDivider(div);
	fLocalPath->SetDivider(div);
	
	bgview->AddChild(fBookmarkName);
	bgview->AddChild(fHostAddress);
	bgview->AddChild(fPort);
	bgview->AddChild(fUsername);
	bgview->AddChild(fPassword);
	bgview->AddChild(fEncoder);
	bgview->AddChild(fRemotePath);
	bgview->AddChild(fLocalPath);
	
	fSaveAndConnectButton = new BButton(BRect(10, 215, 140, 240), "Connect", "Connect", new BMessage(CONNECT_CLICKED));
	bgview->AddChild(fSaveAndConnectButton);
	fSaveAndConnectButton->SetEnabled(false);
	
	fSaveButton = new BButton(BRect(150, 215, 210, 240), "Save", "Save", new BMessage(SAVE_CLICKED));
	bgview->AddChild(fSaveButton);
	fSaveButton->SetEnabled(false);
	
	BButton *cancelButton = new BButton(BRect(220, 215, 280, 240), "Cancel", "Cancel", new BMessage(B_QUIT_REQUESTED));
	bgview->AddChild(cancelButton);
	
	fBookmarkName->SetTarget(this);
	fHostAddress->SetTarget(this);
	fPort->SetTarget(this);
	fUsername->SetTarget(this);
	fPassword->SetTarget(this);
	fPassword->TextView()->HideTyping(true);
	
	AddShortcut('W', 0, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('S', 0, new BMessage(SAVE_CLICKED));
	
	fSaveAndConnectButton->MakeFocus(true);
	fStatus = B_BUSY;
}

TBookmarkWindow::~TBookmarkWindow()
{
}

bool TBookmarkWindow::QuitRequested()
{
	if (fStatus == B_BUSY) {
		fStatus = B_ERROR;
		return false;
	}
	return true;
}

void TBookmarkWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case SAVE_CLICKED:
			Save();
			BMessenger(this).SendMessage(B_QUIT_REQUESTED);
			break;
		case CONNECT_CLICKED:
			Save();
			fStatus = B_OK;
			break;
		case EDIT_CHANGED:
			EditChanged();
			break;
		default: BWindow::MessageReceived(msg);
	}
}

bool TBookmarkWindow::Go(const char *pathName, BEntry *entry)
{
	if (pathName != NULL) Load(pathName);
	BWindow *window = dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));
	this->Show();
	while(this->fStatus == B_BUSY) {
		snooze(1000);
		window->UpdateIfNeeded();
	}
	*entry = fEntry;
	bool result = this->fStatus == B_OK;
	BMessenger(this).SendMessage(B_QUIT_REQUESTED);
	return result;
}

status_t TBookmarkWindow::Save()
{
	BString data;
	data << "host " << fHostAddress->Text() << "\n"
		<< "username " << fUsername->Text() << "\n"
		<< "password " << fPassword->Text() << "\n"
		<< "port " << fPort->Text() << "\n"
		<< "encoder " << fEncoder->Text() << "\n"
		<< "remotepath " << fRemotePath->Text() << "\n"
		<< "localpath " << fLocalPath->Text() << "\n"
		;
	
	if (fEntry.InitCheck() != B_OK) {
		BString path(TFtpPositive::GetBookmarksDir().String());
		path << "/" << fBookmarkName->Text();
		fEntry.SetTo(path.String());
	} else {
		status_t e = fEntry.Rename(fBookmarkName->Text());
		if (e != B_OK) {
			BString emsg;
			emsg << strerror(e);
			if (e == B_FILE_EXISTS) {
				if (fNewBookmark) {
					if ((new BAlert("", emsg.String(), "Overwrite", "Cancel"))->Go() == 1) return B_ERROR;
				}
			} else {
				(new BAlert("", emsg.String(), "Cancel"))->Go();
				return B_ERROR;
			}
		}
	}
	
	BPath path;
	fEntry.GetPath(&path);
	BFile file(path.Path(), B_CREATE_FILE | B_WRITE_ONLY);
	if (file.InitCheck() != B_OK) {
		BString emsg;
		emsg << "Bookmark save failed.\n" << strerror(file.InitCheck());
		(new BAlert("", emsg.String(), "Cancel"))->Go();
		return B_ERROR;
	}
	file.SetSize(0);
	ssize_t s = file.Write(data.String(), data.Length());
	if (s < 0) {
		BString msg("Bookmark save failed.\n");
		msg << strerror(s);
		(new BAlert("", msg.String(), "OK"))->Go();
		return B_ERROR;
	}
	return B_OK;
}

void TBookmarkWindow::EditChanged()
{	
	BString str;
	uint32 p = atoi(fPort->Text());
	if (p == 0) str << "21"; else str << p;
	fPort->SetText(str.String());
	SetTitle(fBookmarkName->Text());
	
	fSaveButton->SetEnabled(
		(strlen(fBookmarkName->Text()) != 0) &&
		(strlen(fHostAddress->Text()) != 0) &&
		(strlen(fUsername->Text()) != 0));
	fSaveAndConnectButton->SetEnabled(fSaveButton->IsEnabled());
}

void TBookmarkWindow::Load(const char *pathName)
{
	TConfigFile config(pathName);
	if (config.Status() != B_OK) {
		BString msg("Bookmark load error.\n");
		msg << strerror(config.Status()) << "\n" << pathName;
		(new BAlert("", msg.String(), "OK"))->Go();
		return;
	}
	fEntry.SetTo(pathName);
	BString host, username, password, port, encoder, remotepath, localpath;
	config.Read("host", &host, "");
	config.Read("username", &username, "");
	config.Read("password", &password, "");
	config.Read("port", &port, "21");
	config.Read("encoder", &encoder, "");
	config.Read("remotepath", &remotepath, "");
	config.Read("localpath", &localpath, TFtpPositive::GetDefaultLocalDir().String());
	
	BPath path(pathName), dir;
	path.GetParent(&dir);
	fBookmarkDir.SetTo(dir.Path());
	
	fBookmarkName->SetText(path.Leaf());
	fHostAddress->SetText(host.String());
	fPort->SetText(port.String());
	fUsername->SetText(username.String());
	fPassword->SetText(password.String());
	fEncoder->SetText(encoder.String());
	fRemotePath->SetText(remotepath.String());
	fLocalPath->SetText(localpath.String());
	
	EditChanged();
	fNewBookmark = false;
}
