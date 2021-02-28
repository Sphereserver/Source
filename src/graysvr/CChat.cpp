// Chat system
#include "graysvr.h"	// predef header.
#include "../network/network.h"

////////////////////////////////////////////////////////////////////////////
// -CChat

bool CChat::CreateChannel(LPCTSTR pszName, LPCTSTR pszPassword, CChatMember *pMember)
{
	ADDTOCALLSTACK("CChat::CreateChannel");
	if ( pMember )
	{
		CClient *pClient = pMember->GetClient();
		if ( pClient && !pClient->IsPriv(PRIV_GM) && !(g_Cfg.m_iChatFlags & CHATF_CHANNELCREATION) )
		{
			CGString sName;
			FormatName(sName, NULL, true);
			pMember->SendChatMsg(CHATMSG_PlayerMessage, sName, " Channel creation is disabled.");
			return false;
		}
	}
	if ( !IsValidName(pszName, false) )
	{
		if ( pMember )
			pMember->SendChatMsg(CHATMSG_InvalidConferenceName);
		return false;
	}
	else if ( FindChannel(pszName) )
	{
		if ( pMember )
			pMember->SendChatMsg(CHATMSG_DuplicatedConferenceName);
		return false;
	}

	CChatChannel *pChannel = new CChatChannel(pszName, pszPassword, !pMember);
	m_Channels.InsertTail(pChannel);
	BroadcastAddChannel(pChannel);
	if ( pMember && (g_Cfg.m_iChatFlags & CHATF_CHANNELMODERATION) )
		pChannel->SetModerator(pMember->GetChatName());
	return true;
}

void CChat::DeleteChannel(CChatChannel *pChannel)
{
	ADDTOCALLSTACK("CChat::DeleteChannel");
	ASSERT(pChannel);
	if ( pChannel->m_bStatic )
		return;
	BroadcastRemoveChannel(pChannel);
	delete pChannel;
}

void CChat::JoinChannel(LPCTSTR pszName, LPCTSTR pszPassword, CChatMember *pMember)
{
	ADDTOCALLSTACK("CChat::JoinChannel");
	ASSERT(pMember);
	CClient *pMemberClient = pMember->GetClient();
	ASSERT(pMemberClient);

	CChatChannel *pNewChannel = FindChannel(pszName);
	if ( !pNewChannel )
	{
		pMemberClient->addChatSystemMessage(CHATMSG_NoConference, pszName);
		return;
	}
	pszName = pNewChannel->m_sName;	// fix case-sensitive mismatch

	CChatChannel *pCurrentChannel = pMember->GetChannel();
	if ( pCurrentChannel && (pCurrentChannel == pNewChannel) )
	{
		pMember->SendChatMsg(CHATMSG_AlreadyInConference, pszName);
		return;
	}
	else if ( !pNewChannel->m_sPassword.IsEmpty() && (!pszPassword || (strcmp(static_cast<LPCTSTR>(pNewChannel->m_sPassword), pszPassword) != 0)) )
	{
		if ( pMemberClient->m_UseNewChatSystem )
		{
			CGString sName;
			FormatName(sName, NULL, true);
			pMember->SendChatMsg(CHATMSG_PlayerMessage, sName, " Your client version can't join channels with password.");
		}
		else
			pMemberClient->addChatSystemMessage(CHATMSG_IncorrectPassword);
		return;
	}
	/*else if ( pNewChannel->m_Members.GetCount() >= BYTE_MAX )
	{
		pMemberClient->addChatSystemMessage(CHATMSG_ConferenceIsFull, pszName);
		return;
	}*/

	// Leave current channel
	if ( pCurrentChannel )
		pCurrentChannel->RemoveMember(pMember);

	// Join the new channel
	pNewChannel->AddMember(pMember);
	pNewChannel->SendMember(pMember);		// send this member to all others clients
	pMemberClient->addChatSystemMessage(CHATCMD_JoinedChannel, pszName);
	if ( !pMemberClient->m_UseNewChatSystem )
		pNewChannel->FillMembersList(pMember);	// fill the members list on this client
}

CChatChannel *CChat::FindChannel(LPCTSTR pszChannel) const
{
	ADDTOCALLSTACK("CChat::FindChannel");
	for ( CChatChannel *pChannel = static_cast<CChatChannel *>(m_Channels.GetHead()); pChannel != NULL; pChannel = pChannel->GetNext() )
	{
		if ( strcmpi(static_cast<LPCTSTR>(pChannel->m_sName), pszChannel) == 0 )
			return pChannel;
	}
	return NULL;
}

void CChat::Action(CClient *pClient, const NCHAR *pszText, int len, CLanguageID lang)
{
	ADDTOCALLSTACK("CChat::Action");
	// ARGS:
	//  len = length of the pszText string in NCHAR's.

	if ( !(g_Cfg.m_iFeatureT2A & FEATURE_T2A_CHAT) )
		return;

	CChatMember *pMe = static_cast<CChatMember *>(pClient);
	CChatChannel *pChannel = pMe->GetChannel();

	TCHAR szFullText[MAX_TALK_BUFFER];
	CvtNUNICODEToSystem(szFullText, sizeof(szFullText), pszText, len);

	TCHAR *pszMsg = szFullText + 1;
	switch ( szFullText[0] )	// the 1st character is a command byte (join channel, leave channel, etc)
	{
		case CHATACT_ChangeChannelPassword:		// client shortcut: /pw
		{
			if ( !pChannel )
				goto NoConference;

			pChannel->ChangePassword(pMe, pszMsg);
			break;
		}
		case CHATACT_LeaveChannel:
		{
			if ( !pChannel )
				goto NoConference;

			pChannel->RemoveMember(pMe);
			break;
		}
		case CHATACT_LeaveChat:
		{
			if ( pChannel )
				pChannel->RemoveMember(pMe);
			break;
		}
		case CHATACT_ChannelMessage:
		{
			if ( pChannel )
			{
				pChannel->MemberTalk(pMe, pClient->m_UseNewChatSystem ? szFullText : pszMsg, lang);
				break;
			}
		NoConference:
			pMe->SendChatMsg(CHATMSG_MustBeInAConference);
			return;
		}
		case CHATACT_JoinChannel:				// client shortcut: /conf
		{
			// Look for second double quote to separate channel from password
			size_t i = 1;
			for ( ; pszMsg[i] != '\0'; ++i )
			{
				if ( pszMsg[i] == '"' )
					break;
			}
			pszMsg[i] = '\0';
			TCHAR *pszPassword = pszMsg + i + 1;
			if ( pszPassword[0] == ' ' )	// skip whitespaces
				++pszPassword;
			JoinChannel(pszMsg + 1, pszPassword, pMe);
			break;
		}
		case CHATACT_CreateChannel:				// client shortcut: /newconf
		{
			TCHAR *pszPassword = NULL;
			size_t iMsgLength = strlen(pszMsg);
			for ( size_t i = 0; i < iMsgLength; ++i )
			{
				if ( pszMsg[i] == '{' )	// there's a password here
				{
					pszMsg[i] = '\0';
					pszPassword = pszMsg + i + 1;
					size_t iPasswordLength = strlen(pszPassword);
					for ( i = 0; i < iPasswordLength; ++i )
					{
						if ( pszPassword[i] == '}' )
						{
							pszPassword[i] = '\0';
							break;
						}
					}
					break;
				}
			}
			if ( CreateChannel(pszMsg, pszPassword, pMe) )
				JoinChannel(pszMsg, pszPassword, pMe);
			break;
		}
		case CHATACT_RenameChannel:				// client shortcut: /rename
		{
			if ( !pChannel )
				goto NoConference;

			pMe->RenameChannel(pszMsg);
			break;
		}
		case CHATACT_PrivateMessage:			// client shortcut: /msg
		{
			if ( !pChannel )
				goto NoConference;

			// Split the recipient from the message (look for a space)
			TCHAR szBuffer[MAX_TALK_BUFFER];
			strncpy(szBuffer, pszMsg, sizeof(szBuffer) - 1);
			szBuffer[sizeof(szBuffer) - 1] = '\0';

			size_t i = 0;
			size_t iBufferLength = strlen(szBuffer);
			for ( ; i < iBufferLength; ++i )
			{
				if ( szBuffer[i] == ' ' )
				{
					szBuffer[i] = '\0';
					break;
				}
			}
			pChannel->PrivateMessage(pMe, szBuffer, szBuffer + i + 1, lang);
			break;
		}
		case CHATACT_AddIgnore:					// client shortcut: +ignore
		{
			pMe->AddIgnore(pszMsg);
			break;
		}
		case CHATACT_RemoveIgnore:				// client shortcut: -ignore
		{
			pMe->RemoveIgnore(pszMsg);
			break;
		}
		case CHATACT_ToggleIgnore:				// client shortcut: /ignore
		{
			pMe->ToggleIgnore(pszMsg);
			break;
		}
		case CHATACT_AddVoice:					// client shortcut: +voice
		{
			if ( !pChannel )
				goto NoConference;

			pChannel->AddVoice(pMe, pszMsg);
			break;
		}
		case CHATACT_RemoveVoice:				// client shortcut: -voice
		{
			if ( !pChannel )
				goto NoConference;

			pChannel->RemoveVoice(pMe, pszMsg);
			break;
		}
		case CHATACT_ToggleVoice:				// client shortcut: /voice
		{
			if ( !pChannel )
				goto NoConference;

			pChannel->ToggleVoice(pMe, pszMsg);
			break;
		}
		case CHATACT_AddModerator:				// client shortcut: +ops
		{
			if ( !pChannel )
				goto NoConference;

			pChannel->AddModerator(pMe, pszMsg);
			break;
		}
		case CHATACT_RemoveModerator:			// client shortcut: -ops
		{
			if ( !pChannel )
				goto NoConference;

			pChannel->RemoveModerator(pMe, pszMsg);
			break;
		}
		case CHATACT_ToggleModerator:			// client shortcut: /ops
		{
			if ( !pChannel )
				goto NoConference;

			pChannel->ToggleModerator(pMe, pszMsg);
			break;
		}
		case CHATACT_EnablePrivateMessages:		// client shortcut: +receive
		{
			pMe->SetReceiving(true);
			break;
		}
		case CHATACT_DisablePrivateMessages:	// client shortcut: -receive
		{
			pMe->SetReceiving(false);
			break;
		}
		case CHATACT_TogglePrivateMessages:		// client shortcut: /receive
		{
			pMe->ToggleReceiving();
			break;
		}
		case CHATACT_ShowCharacterName:			// client shortcut: +showname
		{
			pMe->ShowCharacterName();
			break;
		}
		case CHATACT_HideCharacterName:			// client shortcut: -showname
		{
			pMe->HideCharacterName();
			break;
		}
		case CHATACT_ToggleCharacterName:		// client shortcut: /showname
		{
			pMe->ToggleCharacterName();
			break;
		}
		case CHATACT_WhoIs:						// client shortcut: /whois
		{
			if ( !pChannel )
				goto NoConference;

			pChannel->WhoIs(pMe->GetChatName(), pszMsg);
			break;
		}
		case CHATACT_Kick:						// client shortcut: /kick
		{
			if ( !pChannel )
				goto NoConference;

			CChatMember *pMember = pChannel->FindMember(pszMsg);
			if ( pMember )
				pChannel->KickMember(pMe, pMember);
			else
				pMe->SendChatMsg(CHATMSG_NoPlayer, pszMsg);
			break;
		}
		case CHATACT_EnableDefaultVoice:		// client shortcut: +defaultvoice
		{
			if ( !pChannel )
				goto NoConference;

			pChannel->EnableDefaultVoice(pMe->GetChatName());
			break;
		}
		case CHATACT_DisableDefaultVoice:		// client shortcut: -defaultvoice
		{
			if ( !pChannel )
				goto NoConference;

			pChannel->DisableDefaultVoice(pMe->GetChatName());
			break;
		}
		case CHATACT_ToggleDefaultVoice:		// client shortcut: /defaultvoice
		{
			if ( !pChannel )
				goto NoConference;

			pChannel->ToggleDefaultVoice(pMe->GetChatName());
			break;
		}
		case CHATACT_EmoteMessage:				// client shortcut: /emote or /em
		{
			if ( !pChannel )
				goto NoConference;

			pChannel->Emote(pMe->GetChatName(), pszMsg, lang);
			break;
		}
	}
}

void CChat::QuitChat(CChatMember *pClient)
{
	ADDTOCALLSTACK("CChat::QuitChat");
	// Remove from old channel (if any)

	CChatChannel *pCurrentChannel = pClient->GetChannel();
	if ( pCurrentChannel )
		pCurrentChannel->RemoveMember(pClient);
}

void CChat::FormatName(CGString &sName, const CChatMember *pMember, bool bSystem)	//static
{
	ADDTOCALLSTACK("CChat::FormatName");
	// Format chat name with proper color
	// 0 = Yellow (user)
	// 1 = Purple (moderator)
	// 2 = Blue (muted)
	// 3 = Purple (unused?)
	// 4 = White (me)
	// 5 = Green (system)

	int iColor = 0;
	if ( pMember )
	{
		CChatChannel *pChannel = pMember->GetChannel();
		if ( pChannel )
		{
			LPCTSTR pszName = const_cast<CChatMember *>(pMember)->GetChatName();
			if ( pChannel->IsModerator(pszName) )
				iColor = 1;
			else if ( !pChannel->HasVoice(pszName) )
				iColor = 2;

			sName.Format("%d%s", iColor, pszName);
			return;
		}
	}

	iColor = bSystem ? 5 : 4;
	sName.Format("%d%s", iColor, "SYSTEM");
}

bool CChat::IsValidName(LPCTSTR pszName, bool bPlayer)	//static
{
	ADDTOCALLSTACK("CChat::IsValidName");
	// Channels can have spaces, but not player names

	if ( (strlen(pszName) < 1) || g_Cfg.IsObscene(pszName) || (strcmpi(pszName, "SYSTEM") == 0) )
		return false;

	size_t iNameLength = strlen(pszName);
	for ( size_t i = 0; i < iNameLength; ++i )
	{
		if ( pszName[i] == ' ' )
		{
			if ( bPlayer )
				return false;
			continue;
		}
		if ( !isalnum(pszName[i]) )
			return false;
	}
	return true;
}

void CChat::BroadcastAddChannel(CChatChannel *pChannel)
{
	ADDTOCALLSTACK("CChat::BroadcastAddChannel");
	// Send 'add channel' message to all clients

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
		pClient->addChatSystemMessage(CHATCMD_AddChannel, static_cast<LPCTSTR>(pChannel->m_sName), pClient->m_UseNewChatSystem ? NULL : pChannel->GetPasswordString());
}

void CChat::BroadcastRemoveChannel(CChatChannel *pChannel)
{
	ADDTOCALLSTACK("CChat::BroadcastRemoveChannel");
	// Send 'delete channel' message to all clients

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
		pClient->addChatSystemMessage(CHATCMD_RemoveChannel, static_cast<LPCTSTR>(pChannel->m_sName));
}

////////////////////////////////////////////////////////////////////////////
// -CChatChannel

void CChatChannel::AddMember(CChatMember *pMember)
{
	ADDTOCALLSTACK("CChatChannel::AddMember");
	pMember->SetChannel(this);
	m_Members.Add(pMember);

	LPCTSTR pszName = pMember->GetChatName();
	if ( !IsModerator(pszName) )
	{
		// Check if only moderators have voice by default
		if ( !m_bDefaultVoice )
			SetVoice(pszName);

		// GMs always have moderation privs
		CClient *pClient = pMember->GetClient();
		if ( pClient && pClient->IsPriv(PRIV_GM) )
			SetModerator(pszName);
	}
}

void CChatChannel::RemoveMember(CChatMember *pMember)
{
	ADDTOCALLSTACK("CChatChannel::RemoveMember");
	CClient *pClient = NULL;
	for ( size_t i = 0; i < m_Members.GetCount(); ++i )
	{
		pClient = m_Members[i]->GetClient();
		if ( !pClient )		// auto-remove offline clients
		{
			m_Members[i]->SetChannel(NULL);
			m_Members.RemoveAt(i);
			continue;
		}

		if ( !pClient->m_UseNewChatSystem )
			pClient->addChatSystemMessage(CHATCMD_RemoveMemberFromChannel, pMember->GetChatName());

		if ( m_Members[i] == pMember )
		{
			pClient->addChatSystemMessage(pClient->m_UseNewChatSystem ? CHATCMD_LeftChannel : CHATCMD_ClearMembers, static_cast<LPCTSTR>(m_sName));
			m_Members.RemoveAt(i);
		}
	}

	// Delete the channel if there's no members left
	if ( m_Members.GetCount() <= 0 )
		g_Serv.m_Chats.DeleteChannel(this);

	pMember->SetChannel(NULL);
}

bool CChatChannel::RemoveMember(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatChannel::RemoveMember(2)");

	CChatMember *pMember = FindMember(pszName);
	if ( !pMember )
		return false;
	RemoveMember(pMember);
	return true;
}

void CChatChannel::KickMember(CChatMember *pByMember, CChatMember *pMember)
{
	ADDTOCALLSTACK("CChatChannel::KickMember");
	ASSERT(pMember);

	LPCTSTR pszByName = "SYSTEM";
	if ( pByMember )
	{
		pszByName = pByMember->GetChatName();
		if ( !IsModerator(pszByName) )
		{
			pByMember->SendChatMsg(CHATMSG_MustHaveOps);
			return;
		}
	}

	// Remove from moderators list
	LPCTSTR pszName = pMember->GetChatName();
	if ( IsModerator(pszName) )
	{
		SetModerator(pszName, true);
		SendMember(pMember);
		Broadcast(CHATMSG_PlayerNoLongerModerator, pszName);
		pMember->SendChatMsg(CHATMSG_RemovedListModerators, pszByName);
	}

	pMember->SendChatMsg(CHATMSG_ModeratorHasKicked, pszByName);
	pMember->SendChatMsg(CHATCMD_ClearMembers);
	Broadcast(CHATMSG_PlayerKicked, pszName);
	RemoveMember(pMember);
}

CChatMember *CChatChannel::FindMember(LPCTSTR pszName) const
{
	ADDTOCALLSTACK("CChatChannel::FindMember");

	size_t i = FindMemberIndex(pszName);
	if ( i == m_Members.BadIndex() )
		return NULL;
	return m_Members[i];
}

bool CChatChannel::IsModerator(LPCTSTR pszMember)
{
	ADDTOCALLSTACK("CChatChannel::IsModerator");
	for ( size_t i = 0; i < m_Moderators.GetCount(); ++i )
	{
		if ( m_Moderators[i]->CompareNoCase(pszMember) == 0 )
			return true;
	}
	return false;
}

void CChatChannel::SetModerator(LPCTSTR pszMember, bool bRemoveAccess)
{
	ADDTOCALLSTACK("CChatChannel::SetModerator");
	// Check if they are already a moderator
	for ( size_t i = 0; i < m_Moderators.GetCount(); ++i )
	{
		if ( m_Moderators[i]->CompareNoCase(pszMember) == 0 )
		{
			if ( bRemoveAccess )
				m_Moderators.DeleteAt(i);
			return;
		}
	}
	if ( !bRemoveAccess )
		m_Moderators.Add(new CGString(pszMember));
}

void CChatChannel::AddModerator(CChatMember *pByMember, LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatChannel::AddModerator");

	LPCTSTR pszByName = pByMember->GetChatName();
	if ( !IsModerator(pszByName) )
	{
		pByMember->SendChatMsg(CHATMSG_MustHaveOps);
		return;
	}

	CChatMember *pMember = FindMember(pszName);
	if ( !pMember )
	{
		pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
		return;
	}
	pszName = pMember->GetChatName();	// fix case-sensitive mismatch
	if ( IsModerator(pszName) )
		return;

	SetModerator(pszName);
	SendMember(pMember);	// update name color
	Broadcast(CHATMSG_PlayerIsModerator, pszName);
	pMember->SendChatMsg(CHATMSG_YouAreAModerator, pszByName);
}

void CChatChannel::RemoveModerator(CChatMember *pByMember, LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatChannel::RemoveModerator");

	LPCTSTR pszByName = pByMember->GetChatName();
	if ( !IsModerator(pszByName) )
	{
		pByMember->SendChatMsg(CHATMSG_MustHaveOps);
		return;
	}

	CChatMember *pMember = FindMember(pszName);
	if ( !pMember )
	{
		pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
		return;
	}
	pszName = pMember->GetChatName();	// fix case-sensitive mismatch
	if ( !IsModerator(pszName) )
		return;

	SetModerator(pszName, true);
	SendMember(pMember);	// update name color
	Broadcast(CHATMSG_PlayerNoLongerModerator, pszName);
	pMember->SendChatMsg(CHATMSG_RemovedListModerators, pszByName);
}

void CChatChannel::ToggleModerator(CChatMember *pByMember, LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatChannel::ToggleModerator");
	if ( !IsModerator(pszName) )
		AddModerator(pByMember, pszName);
	else
		RemoveModerator(pByMember, pszName);
}

bool CChatChannel::HasVoice(LPCTSTR pszMember)
{
	ADDTOCALLSTACK("CChatChannel::HasVoice");
	if ( !m_bDefaultVoice && !IsModerator(pszMember) )
		return false;
	for ( size_t i = 0; i < m_Muted.GetCount(); ++i )
	{
		if ( m_Muted[i]->CompareNoCase(pszMember) == 0 )
			return false;
	}
	return true;
}

void CChatChannel::SetVoice(LPCTSTR pszName, bool bRemoveAccess)
{
	ADDTOCALLSTACK("CChatChannel::SetVoice");
	// Check if they have no voice already
	for ( size_t i = 0; i < m_Muted.GetCount(); ++i )
	{
		if ( m_Muted[i]->CompareNoCase(pszName) == 0 )
		{
			if ( !bRemoveAccess )
				m_Muted.DeleteAt(i);
			return;
		}
	}
	if ( bRemoveAccess )
		m_Muted.Add(new CGString(pszName));
}

void CChatChannel::AddVoice(CChatMember *pByMember, LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatChannel::AddVoice");

	LPCTSTR pszByName = pByMember->GetChatName();
	if ( !IsModerator(pszByName) )
	{
		pByMember->SendChatMsg(CHATMSG_MustHaveOps);
		return;
	}

	CChatMember *pMember = FindMember(pszName);
	if ( !pMember )
	{
		pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
		return;
	}
	pszName = pMember->GetChatName();	// fix case-sensitive mismatch
	if ( HasVoice(pszName) )
		return;

	SetVoice(pszName);
	SendMember(pMember);	// update name color
	pMember->SendChatMsg(CHATMSG_ModeratorHasGrantedSpeakPriv, pszByName);
	Broadcast(CHATMSG_PlayerNowSpeaking, pszName);
}

void CChatChannel::RemoveVoice(CChatMember *pByMember, LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatChannel::RemoveVoice");

	LPCTSTR pszByName = pByMember->GetChatName();
	if ( !IsModerator(pszByName) )
	{
		pByMember->SendChatMsg(CHATMSG_MustHaveOps);
		return;
	}

	CChatMember *pMember = FindMember(pszName);
	if ( !pMember )
	{
		pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
		return;
	}
	pszName = pMember->GetChatName();	// fix case-sensitive mismatch
	if ( !HasVoice(pszName) )
		return;

	SetVoice(pszName, true);
	SendMember(pMember);	// update name color
	pMember->SendChatMsg(CHATMSG_ModeratorHasRemovedSpeakPriv, pszByName);
	Broadcast(CHATMSG_PlayerNoSpeaking, pszName);
}

void CChatChannel::ToggleVoice(CChatMember *pByMember, LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatChannel::ToggleVoice");
	if ( !HasVoice(pszName) )	// this also returns true if this person is not in the channel
		AddVoice(pByMember, pszName);
	else
		RemoveVoice(pByMember, pszName);
}

void CChatChannel::EnableDefaultVoice(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatChannel::EnableDefaultVoice");
	if ( !m_bDefaultVoice )
		ToggleDefaultVoice(pszName);
}

void CChatChannel::DisableDefaultVoice(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatChannel::DisableDefaultVoice");
	if ( m_bDefaultVoice )
		ToggleDefaultVoice(pszName);
}

void CChatChannel::ToggleDefaultVoice(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatChannel::ToggleDefaultVoice");
	if ( !IsModerator(pszName) )
	{
		FindMember(pszName)->SendChatMsg(CHATMSG_MustHaveOps);
		return;
	}
	m_bDefaultVoice = !m_bDefaultVoice;
	Broadcast(m_bDefaultVoice ? CHATMSG_EveryoneSpeakingPrivByDefault : CHATMSG_ModeratorSpeakingPrivByDefault);
}

void CChatChannel::MemberTalk(CChatMember *pByMember, LPCTSTR pszText, CLanguageID lang)
{
	ADDTOCALLSTACK("CChatChannel::MemberTalk");

	LPCTSTR pszName = pByMember->GetChatName();
	if ( !HasVoice(pszName) )
	{
		pByMember->SendChatMsg(CHATMSG_RevokedSpeaking);
		return;
	}
	Broadcast(CHATMSG_PlayerMessage, pszName, pszText, lang);
}

void CChatChannel::Emote(LPCTSTR pszBy, LPCTSTR pszMsg, CLanguageID lang)
{
	ADDTOCALLSTACK("CChatChannel::Emote");
	if ( HasVoice(pszBy) )
		Broadcast(CHATMSG_PlayerEmote, pszBy, pszMsg, lang);
	else
		FindMember(pszBy)->SendChatMsg(CHATMSG_RevokedSpeaking);
}

void CChatChannel::PrivateMessage(CChatMember *pFrom, LPCTSTR pszTo, LPCTSTR pszMsg, CLanguageID lang)
{
	ADDTOCALLSTACK("CChatChannel::PrivateMessage");
	CChatMember *pTo = FindMember(pszTo);
	if ( !pTo )
	{
		pFrom->SendChatMsg(CHATMSG_NoPlayer, pszTo);
		return;
	}
	if ( !pTo->m_bReceiving )
	{
		pFrom->SendChatMsg(CHATMSG_PlayerNotReceivingPrivateMessages, pszTo);
		return;
	}

	// Members without voice can't send private messages on channel, but they still allowed to send private messages to moderators
	if ( !HasVoice(pFrom->GetChatName()) && !IsModerator(pszTo) )
	{
		pFrom->SendChatMsg(CHATMSG_RevokedSpeaking);
		return;
	}

	if ( pTo->IsIgnoring(pFrom->GetChatName()) )
	{
		pFrom->SendChatMsg(CHATMSG_PlayerIsIgnoring, pszTo);
		return;
	}

	CGString sName;
	g_Serv.m_Chats.FormatName(sName, pFrom);
	pFrom->SendChatMsg(CHATMSG_PlayerPrivateMessage, sName, pszMsg, lang);
	if ( pTo != pFrom )
		pTo->SendChatMsg(CHATMSG_PlayerPrivateMessage, sName, pszMsg, lang);
}

void CChatChannel::WhoIs(LPCTSTR pszBy, LPCTSTR pszMember)
{
	ADDTOCALLSTACK("CChatChannel::WhoIs");
	CChatMember *pMemberBy = FindMember(pszBy);
	CChatMember *pMemberTarg = FindMember(pszMember);
	CChar *pCharTarg = (pMemberTarg && pMemberTarg->GetClient()) ? pMemberTarg->GetClient()->GetChar() : NULL;

	if ( !pCharTarg )
		pMemberBy->SendChatMsg(CHATMSG_NoPlayer, pszMember);
	else if ( pMemberTarg->m_bAllowWhoIs )
		pMemberBy->SendChatMsg(CHATMSG_PlayerKnownAs, pszMember, pCharTarg->GetName());
	else
		pMemberBy->SendChatMsg(CHATMSG_PlayerIsAnonymous, pszMember);
}

void CChatChannel::RenameChannel(CChatMember *pByMember, LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatChannel::RenameChannel");

	if ( !g_Serv.m_Chats.IsValidName(pszName, false) )
	{
		pByMember->SendChatMsg(CHATMSG_InvalidConferenceName);
		return;
	}
	if ( g_Serv.m_Chats.FindChannel(pszName) )
	{
		pByMember->SendChatMsg(CHATMSG_DuplicatedConferenceName);
		return;
	}

	Broadcast(CHATMSG_ConferenceRenamed, static_cast<LPCTSTR>(m_sName), pszName);
	g_Serv.m_Chats.BroadcastRemoveChannel(this);
	m_sName = pszName;
	Broadcast(CHATCMD_JoinedChannel, pszName);
	g_Serv.m_Chats.BroadcastAddChannel(this);
}

void CChatChannel::ChangePassword(CChatMember *pByMember, LPCTSTR pszPassword)
{
	ADDTOCALLSTACK("CChatChannel::ChangePassword");

	if ( !IsModerator(pByMember->GetChatName()) )
	{
		pByMember->SendChatMsg(CHATMSG_MustHaveOps);
		return;
	}
	m_sPassword = pszPassword;
	g_Serv.m_Chats.BroadcastAddChannel(pByMember->GetChannel());
	Broadcast(CHATMSG_PasswordChanged);
}

size_t CChatChannel::FindMemberIndex(LPCTSTR pszName) const
{
	ADDTOCALLSTACK("CChatChannel::FindMemberIndex");
	if ( pszName != NULL )
	{
		for ( size_t i = 0; i < m_Members.GetCount(); ++i )
		{
			if ( strcmpi(m_Members[i]->GetChatName(), pszName) == 0 )
				return i;
		}
	}
	return m_Members.BadIndex();
}

void CChatChannel::Broadcast(CHATMSG_TYPE iType, LPCTSTR pszName, LPCTSTR pszText, CLanguageID lang, bool bOverride)
{
	ADDTOCALLSTACK("CChatChannel::Broadcast");

	CChatMember *pSendingMember = FindMember(pszName);
	CGString sName;
	if ( (iType >= CHATMSG_PlayerMessage) && (iType <= CHATMSG_PlayerPrivateMessage) )
		g_Serv.m_Chats.FormatName(sName, pSendingMember, bOverride);
	else
		sName = pszName;

	for ( size_t i = 0; i < m_Members.GetCount(); ++i )
	{
		if ( m_Members[i]->IsIgnoring(pszName) )	// don't receive messages from players being ignored
		{
			if ( pSendingMember && (iType == CHATMSG_PlayerPrivateMessage) )	// if it's a private message, tell the sender that he's being ignored
				pSendingMember->SendChatMsg(CHATMSG_PlayerIsIgnoring, m_Members[i]->GetChatName());
			continue;
		}
		m_Members[i]->SendChatMsg(iType, sName, pszText, lang);
	}
}

void CChatChannel::SendMember(CChatMember *pMember, CChatMember *pToMember)
{
	ADDTOCALLSTACK("CChatChannel::SendMember");
	CGString sName;
	g_Serv.m_Chats.FormatName(sName, pMember);

	CClient *pClient = NULL;
	if ( pToMember )
	{
		// If pToMember is specified, send only to this member
		pClient = pToMember->GetClient();
		if ( pClient && pClient->m_UseNewChatSystem )
			return;
		if ( pToMember->IsIgnoring(pMember->GetChatName()) )
			return;
		pToMember->SendChatMsg(CHATCMD_AddMemberToChannel, sName);
	}
	else
	{
		// If no pToMember is specified, send to all members
		for ( size_t i = 0; i < m_Members.GetCount(); ++i )
		{
			pClient = m_Members[i]->GetClient();
			if ( pClient && pClient->m_UseNewChatSystem )
				continue;
			if ( m_Members[i]->IsIgnoring(pMember->GetChatName()) )
				continue;
			m_Members[i]->SendChatMsg(CHATCMD_AddMemberToChannel, sName);
		}
	}
}

void CChatChannel::FillMembersList(CChatMember *pMember)
{
	ADDTOCALLSTACK("CChatChannel::FillMembersList");
	for ( size_t i = 0; i < m_Members.GetCount(); ++i )
	{
		if ( pMember->IsIgnoring(m_Members[i]->GetChatName()) )
			continue;
		CGString sName;
		g_Serv.m_Chats.FormatName(sName, m_Members[i]);
		pMember->SendChatMsg(CHATCMD_AddMemberToChannel, sName);
	}
}

////////////////////////////////////////////////////////////////////////////
// -CChatMember

CChatMember::~CChatMember()
{
	if ( m_bChatActive )
		g_Serv.m_Chats.QuitChat(this);
}

void CChatMember::addChatWindow()
{
	ADDTOCALLSTACK("CChatMember::addChatWindow");
	// Called from Event_ChatButton

	CClient *pClient = GetClient();
	if ( !pClient || (!pClient->m_UseNewChatSystem && m_bChatActive) )
		return;

	// Open chat window (old chat system only)
	// On new chat system this is not needed because the chat button is hardcoded on client-side, and
	// PacketChatButton packet is sent by client after login complete only to get initial channel list
	if ( !pClient->m_UseNewChatSystem )
		pClient->addChatSystemMessage(CHATCMD_OpenChatWindow, GetChatName());

	// Send channel names
	for ( CChatChannel *pChannel = static_cast<CChatChannel *>(g_Serv.m_Chats.m_Channels.GetHead()); pChannel != NULL; pChannel = pChannel->GetNext() )
	{
		pClient->addChatSystemMessage(CHATCMD_AddChannel, pChannel->m_sName, pClient->m_UseNewChatSystem ? NULL : pChannel->GetPasswordString());
		if ( (g_Cfg.m_iChatFlags & CHATF_AUTOJOIN) && pChannel->m_bStatic && !GetChannel() )
			g_Serv.m_Chats.JoinChannel(pChannel->m_sName, NULL, this);
	}
}

void CChatMember::SendChatMsg(CHATMSG_TYPE iType, LPCTSTR pszName1, LPCTSTR pszName2, CLanguageID lang)
{
	ADDTOCALLSTACK("CChatMember::SendChatMsg");
	CClient *pClient = GetClient();
	ASSERT(pClient);
	pClient->addChatSystemMessage(iType, pszName1, pszName2, lang);
}

void CChatMember::ToggleReceiving()
{
	ADDTOCALLSTACK("CChatMember::ToggleReceiving");
	m_bReceiving = !m_bReceiving;
	SendChatMsg(m_bReceiving ? CHATMSG_ReceivingPrivateMessages : CHATMSG_NoLongerReceivingPrivateMessages);
}

void CChatMember::ShowCharacterName()
{
	ADDTOCALLSTACK("CChatMember::ShowCharacterName");
	if ( m_bAllowWhoIs )
		return;

	m_bAllowWhoIs = true;
	SendChatMsg(CHATMSG_ShowingName);
}

void CChatMember::HideCharacterName()
{
	ADDTOCALLSTACK("CChatMember::HideCharacterName");
	if ( !m_bAllowWhoIs )
		return;

	m_bAllowWhoIs = false;
	SendChatMsg(CHATMSG_NoLongerShowingName);
}

void CChatMember::ToggleCharacterName()
{
	ADDTOCALLSTACK("CChatMember::ToggleCharacterName");
	m_bAllowWhoIs = !m_bAllowWhoIs;
	SendChatMsg(m_bAllowWhoIs ? CHATMSG_ShowingName : CHATMSG_NoLongerShowingName);
}

void CChatMember::RenameChannel(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatMember::RenameChannel");
	CChatChannel *pChannel = GetChannel();
	if ( !pChannel )
		SendChatMsg(CHATMSG_MustBeInAConference);
	else if ( !pChannel->IsModerator(GetChatName()) )
		SendChatMsg(CHATMSG_MustHaveOps);
	else
		pChannel->RenameChannel(this, pszName);
}

void CChatMember::AddIgnore(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatMember::AddIgnore");
	if ( IsIgnoring(pszName) )
		SendChatMsg(CHATMSG_AlreadyIgnoringPlayer, pszName);
	else
		ToggleIgnore(pszName);
}

void CChatMember::RemoveIgnore(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatMember::RemoveIgnore");
	if ( !IsIgnoring(pszName) )
		SendChatMsg(CHATMSG_NotIgnoring, pszName);
	else
		ToggleIgnore(pszName);
}

void CChatMember::ToggleIgnore(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CChatMember::ToggleIgnore");
	size_t i = FindIgnoringIndex(pszName);
	if ( i != m_IgnoredMembers.BadIndex() )
	{
		// Remove member from ignore list
		ASSERT(m_IgnoredMembers.IsValidIndex(i));
		m_IgnoredMembers.DeleteAt(i);
		SendChatMsg(CHATMSG_NoLongerIgnoring, pszName);

		if ( !m_IgnoredMembers.GetCount() )
			SendChatMsg(CHATMSG_NoLongerIgnoringAnyone);

		// Show member name on members list again
		if ( m_pChannel )
		{
			CChatMember *pMember = m_pChannel->FindMember(pszName);
			if ( pMember )
				m_pChannel->SendMember(pMember, this);
		}
		return;
	}
	else
	{
		// Check if ignore list reached max limit
		if ( m_IgnoredMembers.GetCount() >= 30 )
		{
			SendChatMsg(CHATMSG_AlreadyIgnoringMax, pszName);
			return;
		}

		// Add member on ignore list
		CGString *sName = new CGString(pszName);
		m_IgnoredMembers.Add(sName);
		SendChatMsg(CHATMSG_NowIgnoring, pszName);	// this message will also hide member name on members list
	}
}

size_t CChatMember::FindIgnoringIndex(LPCTSTR pszName) const
{
	ADDTOCALLSTACK("CChatMember::FindIgnoringIndex");
	if ( pszName != NULL )
	{
		for ( size_t i = 0; i < m_IgnoredMembers.GetCount(); ++i )
		{
			if ( m_IgnoredMembers[i]->CompareNoCase(pszName) == 0 )
				return i;
		}
	}
	return m_IgnoredMembers.BadIndex();
}

CClient *CChatMember::GetClient()
{
	ADDTOCALLSTACK("CChatMember::GetClient");
	return static_cast<CClient *>(this);
}

LPCTSTR CChatMember::GetChatName()
{
	ADDTOCALLSTACK("CChatMember::GetChatName");
	CClient *pClient = GetClient();
	if ( pClient )
	{
		if ( !pClient->m_UseNewChatSystem && (g_Cfg.m_iChatFlags & CHATF_CUSTOMNAMES) )
			return pClient->m_pAccount->m_sChatName;

		CChar *pChar = pClient->GetChar();
		if ( pChar )
			return pChar->GetName();
	}
	return "<NA>";
}
