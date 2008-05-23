//
// CChat.CPP
// Copyright Menace Software (www.menasoft.com).
//
// Chat system
//

#include "graysvr.h"	// predef header.

////////////////////////////////////////////////////////////////////////////
// -CChat

void CChat::EventMsg( CClient * pClient, const NCHAR * pszText, int len, CLanguageID lang ) // Text from a client
{
	ADDTOCALLSTACK("CChat::EventMsg");
	// ARGS:
	//  len = length of the pszText string in NCHAR's.
	//

	CChatChanMember * pMe = pClient;
	ASSERT(pMe);
	CChatChannel * pChannel =  pMe->GetChannel();

	TCHAR szText[MAX_TALK_BUFFER * 2];
	CvtNUNICODEToSystem( szText, sizeof(szText), pszText, len );

	// The 1st character is a command byte, join channel, private message someone, etc etc
	TCHAR * szMsg = szText+1;
	switch ( szText[0] )
	{
	case 'a':	// a = client typed a plain message in the text entry area
	{
		// Check for a chat command here
		if (szMsg[0] == '/')
		{
			DoCommand(pMe, szMsg + 1);
			break;
		}
		if (!pChannel)
		{
not_in_a_channel:
			pMe->SendChatMsg(CHATMSG_MustBeInAConference);
			return;
		}
		// Not a chat command, must be speech
		pChannel->MemberTalk(pMe, szMsg, lang);
		break;
	};
	case 'A':	// A = change the channel password
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->ChangePassword(pMe, szMsg);
		break;
	};
	case 'b':	// b = client joining an existing channel
	{
		// Look for second double quote to separate channel from password
		int i = 1;
		for(; i < strlen(szMsg); i++)
			if (szMsg[i] == '"')
				break;
		szMsg[i] = 0;
		char * pszPassword = szMsg + i + 1;
		if (pszPassword[0] == ' ') // skip leading space if any
			pszPassword++;
		JoinChannel( pMe, szMsg + 1, pszPassword);
		break;
	};
	case 'c':	// c = client creating (and joining) new channel
	{
		char * pszPassword = NULL;
		for (int i = 0; i < strlen(szMsg); i++)
		{
			if (szMsg[i] == '{') // OK, there's a password here
			{
				szMsg[i] = 0;
				pszPassword = szMsg + i + 1;
				for(i = 0; i < strlen(pszPassword); i++)
				{
					if (pszPassword[i] == '}')
					{
						pszPassword[i] = 0;
						break;
					}
				}
				break;
			}
		}
		CreateJoinChannel(pMe, szMsg, pszPassword);
		break;
	};
	case 'd':	// d = (/rename x) rename conference
	{
		if (!pChannel)
			goto not_in_a_channel;
		pMe->RenameChannel(szMsg);
		break;
	};
	case 'e':	// e = Send a private message to ....
	{
		if (!pChannel)
			goto not_in_a_channel;
		char buffer[2048];
		strcpy(buffer, szMsg);
		// Separate the recipient from the message (look for a space)
		int i=0;
		for (; i < strlen(buffer); i++)
		{
			if (buffer[i] == ' ')
			{
				buffer[i] = 0;
				break;
			}
		}
		pChannel->SendPrivateMessage(pMe, buffer, buffer+i+1);
		break;
	};
	case 'f':	// f = (+ignore) ignore this person
	{
		pMe->Ignore(szMsg);
		break;
	};
	case 'g':	// g = (-ignore) don't ignore this person
	{
		pMe->DontIgnore(szMsg);
		break;
	};
	case 'h':	// h = toggle ignoring this person
	{
		pMe->ToggleIgnore(szMsg);
		break;
	};
	case 'i':	// i = grant speaking privs to this person
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->GrantVoice(pMe, szMsg);
		break;
	};
	case 'j':	// j = remove speaking privs from this person
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->RevokeVoice(pMe, szMsg);
		break;
	};
	case 'k':	// k = (/voice) toggle voice status
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->ToggleVoice(pMe, szMsg);
		break;
	};
	case 'l':	// l = grant moderator status to this person
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->GrantModerator(pMe, szMsg);
		break;
	};
	case 'm':	// m = remove moderator status from this person
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->RevokeModerator(pMe, szMsg);
		break;
	};
	case 'n':	// m = toggle the moderator status for this person
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->ToggleModerator(pMe, szMsg);
		break;
	}
	case 'o':	// o = turn on receiving private messages
	{
		pMe->SetReceiving(true);
		break;
	}
	case 'p':	// p = turn off receiving private messages
	{
		pMe->SetReceiving(false);
		break;
	}
	case 'q':	// q = toggle receiving messages
	{
		pMe->ToggleReceiving();
		break;
	};
	case 'r':	// r = (+showname) turn on showing character name
	{
		pMe->PermitWhoIs();
		break;
	};
	case 's':	// s = (-showname) turn off showing character name
	{
		pMe->ForbidWhoIs();
		break;
	};
	case 't':	// t = toggle showing character name
	{
		pMe->ToggleWhoIs();
		break;
	};
	case 'u':	// u = who is this player
	{
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->WhoIs(pMe->GetChatName(), szMsg);
		break;
	};
	case 'v':	// v = kick this person out of the conference
	{
		if (!pChannel)
			goto not_in_a_channel;

		CChatChanMember * pMember = pChannel->FindMember(szMsg);
		if (!pMember)
		{
			pMe->SendChatMsg(CHATMSG_NoPlayer, szMsg);
			break;
		}

		pChannel->KickMember(pMe, pMember);
		// If noone is left, tell the chat system to
		// delete it from memory (you can kick yourself)
		if (pChannel->m_Members.GetCount() <= 0) // Kicked self
		{
			DeleteChannel(pChannel);
		}
		break;
	};
	case 'X':	// X = client quit chat
		QuitChat(pClient);
		break;
	case 'w':	// w = (+defaultvoice) make moderators be the only ones with a voice by default
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->DisableVoiceDefault(pMe->GetChatName());
		break;
	case 'x':	// x = (-defaultvoice) give everyone a voice by default
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->EnableVoiceDefault(pMe->GetChatName());
		break;
	case 'y':	// y = (/defaultvoice) toggle
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->ToggleVoiceDefault(pMe->GetChatName());
		break;
	case 'z':	// z = emote
		if (!pChannel)
			goto not_in_a_channel;
		pChannel->Emote(pMe->GetChatName(), szMsg, lang );
		break;
	};
}

void CChat::QuitChat(CChatChanMember * pClient)
{
	ADDTOCALLSTACK("CChat::QuitChat");
	// Are we in a channel now?
	CChatChannel * pCurrentChannel = pClient->GetChannel();
	// Remove from old channel (if any)
	if (pCurrentChannel)
	{
		// Remove myself from the channels list of members
		pCurrentChannel->RemoveMember( pClient );

		// Am I the last one here? Delete it from all other clients?
		if (pCurrentChannel->m_Members.GetCount() <= 0)
		{
			// If noone is left, tell the chat system to delete it from memory
			DeleteChannel(pCurrentChannel);
		}
	}
	// Now tell the chat system you left

	pClient->SetChatInactive();
}

void CChat::DoCommand(CChatChanMember * pBy, LPCTSTR szMsg)
{
	ADDTOCALLSTACK("CChat::DoCommand");
	static LPCTSTR const sm_szCmd_Chat[] =
	{
		"ALLKICK",
		"BC",
		"BCALL",
		"CHATSOK",
		"CLEARIGNORE",
		"KILLCHATS",
		"NOCHATS",
		"SYSMSG",
		"WHEREIS",
	};

	ASSERT(pBy);

	char buffer[2048];
	char * pszCommand;
	char * pszText;
	strcpy(buffer, szMsg);
	pszCommand = buffer;
	for (int i = 0; i < strlen(pszCommand); i++)
	{
		ASSERT( i<COUNTOF(buffer));
		if (pszCommand[i] == ' ')
		{
			pszCommand[i] = 0;
			pszText = pszCommand + i + 1;
		}
	}

	CGString sFrom;
	CChatChannel * pChannel = pBy->GetChannel();

	//static bool fFlipper = false;
	//static int iCounter = 0;

	switch ( FindTableSorted( pszCommand, sm_szCmd_Chat, COUNTOF(sm_szCmd_Chat)))
	{
		case 0: // "ALLKICK"
		{
			if (!pChannel)
			{
				pBy->SendChatMsg(CHATMSG_MustBeInAConference);
				return;
			}
			if (!pChannel->IsModerator(pBy->GetChatName()))
			{
				pBy->SendChatMsg(CHATMSG_MustHaveOps);
				return;
			}
			pChannel->KickAll(pBy);
			DecorateName(sFrom, NULL, true);
			pBy->SendChatMsg(CHATMSG_PlayerTalk, sFrom, "All members have been kicked!", "");
			return;
		}
		case 1: // "BC"
		{
			if ( ! pBy->GetClient()->IsPriv( PRIV_GM ))
			{
	need_gm_privs:
				DecorateName(sFrom, NULL, true);
				pBy->SendChatMsg(CHATMSG_PlayerTalk, sFrom, "You need to have GM privs to use this command.");
				return;
			}
			Broadcast(pBy, pszText);
			return;
		}
		case 2: // "BCALL"
		{
			if ( ! pBy->GetClient()->IsPriv( PRIV_GM ))
				goto need_gm_privs;
			Broadcast(pBy, pszText, "", true);
			return;
		}
		case 3: // "CHATSOK"
		{
			if ( ! pBy->GetClient()->IsPriv( PRIV_GM ))
				goto need_gm_privs;
			if (!m_fChatsOK)
			{
				m_fChatsOK = true;
				Broadcast(NULL, "Conference creation is enabled.");
			}
			return;
		}
		case 4: // "CLEARIGNORE"
		{
			pBy->ClearIgnoreList();
			return;
		}
		case 5: // "KILLCHATS"
		{
			if ( ! pBy->GetClient()->IsPriv( PRIV_GM ))
				goto need_gm_privs;
			KillChannels();
			return;
		}
		case 6: // "NOCHATS"
		{
			if ( ! pBy->GetClient()->IsPriv( PRIV_GM ))
				goto need_gm_privs;
			if (m_fChatsOK)
			{
				Broadcast(NULL, "Conference creation is now disabled.");
				m_fChatsOK = false;
			}
			return;
		}
		case 7: // "SYSMSG"
		{
			if ( ! pBy->GetClient()->IsPriv( PRIV_GM ))
				goto need_gm_privs;
			Broadcast(NULL, pszText, "", true);
			return;
		}
		case 8:	// "WHEREIS"
		{
			WhereIs(pBy, pszText);
			return;
		}
		default:
		{
			TCHAR *pszMsg = Str_GetTemp();
			sprintf(pszMsg, "Unknown command: '%s'", pszCommand);
			CGString sFrom;
			DecorateName(sFrom, NULL, true);
			pBy->SendChatMsg(CHATMSG_PlayerTalk, sFrom, pszMsg);
			return;
		}
	}
}

void CChat::KillChannels()
{
	ADDTOCALLSTACK("CChat::KillChannels");
	CChatChannel * pChannel = GetFirstChannel();
	// First /kick everyone
	for ( ; pChannel != NULL; pChannel = pChannel->GetNext())
		pChannel->KickAll();
	m_Channels.Empty();
};

void CChat::WhereIs(CChatChanMember * pBy, LPCTSTR pszName ) const
{
	ADDTOCALLSTACK("CChat::WhereIs");
	CClient * pClient = g_Serv.GetClientHead();
	for ( ; pClient; pClient = pClient->GetNext())
	{
		if ( ! strcmp( pClient->GetChatName(), pszName))
			continue;

		TCHAR *pszMsg = Str_GetTemp();
		if (! pClient->IsChatActive() || !pClient->GetChannel())
			sprintf(pszMsg, "%s is not currently in a conference.", pszName);
		else
			sprintf(pszMsg, "%s is in conference '%s'.", (LPCTSTR) pszName, (LPCTSTR) pClient->GetChannel()->GetName());
		CGString sFrom;
		DecorateName(sFrom, NULL, true);
		pBy->SendChatMsg(CHATMSG_PlayerTalk, sFrom, pszMsg);
		return;
	}

	pBy->SendChatMsg(CHATMSG_NoPlayer, pszName);
}

void CChat::DeleteChannel(CChatChannel * pChannel)
{
	ADDTOCALLSTACK("CChat::DeleteChannel");
	SendDeleteChannel(pChannel);	// tell everyone about it first.
	delete pChannel;
}

void CChat::SendDeleteChannel(CChatChannel * pChannel)
{
	ADDTOCALLSTACK("CChat::SendDeleteChannel");
	// Send a delete channel name message to all clients using the chat system
	CClient * pClient = g_Serv.GetClientHead();
	for ( ; pClient; pClient = pClient->GetNext())
	{
		if ( ! pClient->IsChatActive())
			continue;
		pClient->addChatSystemMessage(CHATMSG_RemoveChannelName, pChannel->GetName());
	}
}

bool CChat::IsValidName( const char * pszName, bool fPlayer ) // static
{
	ADDTOCALLSTACK("CChat::IsValidName");
	// Channels can have spaces, but not player names
	if (strlen(pszName) < 1)
		return false;
	if (strcmp(pszName, "SYSTEM") == 0)
		return false;
	for (int i = 0; i < strlen(pszName); i++)
	{
		if ( pszName[i] == ' ' )
		{
			if (fPlayer)
				return false;
			continue;
		}
		if ( ! isalnum(pszName[i]))
			return( false );
	}
	return true;
}

void CChat::SendNewChannel(CChatChannel * pNewChannel)
{
	ADDTOCALLSTACK("CChat::SendNewChannel");
	// Send this new channel name to all clients using the chat system
	CClient * pClient = g_Serv.GetClientHead();
	for ( ; pClient; pClient = pClient->GetNext())
	{
		if ( ! pClient->IsChatActive())
			continue;
		pClient->addChatSystemMessage(CHATMSG_SendChannelName, pNewChannel->GetName(), pNewChannel->GetModeString());
	}
}

void CChat::DecorateName(CGString &sName, const CChatChanMember * pMember, bool fSystem) // static
{
	ADDTOCALLSTACK("CChat::DecorateName");
	CChatChannel * pChannel = NULL;
	if (pMember)
		pChannel = pMember->GetChannel();
	// 0 = yellow
	// 1 = purple
	// 2 = blue
	// 3 = purple
	// 4 = white
	// 5 = green
	int iResult = 0;
	if (!pMember || !pChannel) // Must be a system command if these are invalid
	{
		if (fSystem)
			iResult = 5;
		else
			iResult = 4;
	}
	else if (pChannel->IsModerator(pMember->GetChatName()))
		iResult = 1;
	else if (!pChannel->HasVoice(pMember->GetChatName()))
		iResult = 2;

	if (!pMember || !pChannel)
		sName.Format("%i%s", iResult, "SYSTEM");
	else
		sName.Format("%i%s", iResult, (LPCTSTR) pMember->GetChatName());
}

void CChat::Broadcast(CChatChanMember * pFrom, LPCTSTR pszText, CLanguageID lang, bool fOverride)
{
	ADDTOCALLSTACK("CChat::Broadcast");
	CClient * pClient = g_Serv.GetClientHead();
	for ( ; pClient; pClient = pClient->GetNext())
	{
		if ( ! pClient->IsChatActive())
			continue;
		if ( fOverride || ( ! fOverride && pClient->IsReceivingAllowed()))
		{
			CGString sName;
			DecorateName(sName, pFrom, fOverride);
			pClient->SendChatMsg(CHATMSG_PlayerTalk, sName, pszText, lang );
		}
	}
}

void CChat::CreateJoinChannel(CChatChanMember * pByMember, LPCTSTR pszName, LPCTSTR pszPassword)
{
	ADDTOCALLSTACK("CChat::CreateJoinChannel");
	if ( ! IsValidName( pszName, false ))
	{
		pByMember->GetClient()->addChatSystemMessage( CHATMSG_InvalidConferenceName );
	}
	else if (IsDuplicateChannelName(pszName))
	{
		pByMember->GetClient()->addChatSystemMessage( CHATMSG_AlreadyAConference );
	}
	else
	{
		if ( CreateChannel(pszName, ((pszPassword != NULL) ? pszPassword : ""), pByMember))
			JoinChannel(pByMember, pszName, ((pszPassword != NULL) ? pszPassword : ""));
	}
}

bool CChat::CreateChannel(const char * pszName, const char * pszPassword, CChatChanMember * pMember)
{
	ADDTOCALLSTACK("CChat::CreateChannel");
	if (!m_fChatsOK)
	{
		CGString sName;
		DecorateName(sName, NULL, true);
		pMember->SendChatMsg(CHATMSG_PlayerTalk, sName, "Conference creation is disabled.");
		return false;
	}
	CChatChannel * pChannel = new CChatChannel( pszName, pszPassword );
	m_Channels.InsertTail( pChannel );
	pChannel->SetModerator(pMember->GetChatName());
	// Send all clients with an open chat window the new channel name
	SendNewChannel(pChannel);
	return true;
}

bool CChat::JoinChannel(CChatChanMember * pMember, const char * pszChannel, const char * pszPassword)
{
	ADDTOCALLSTACK("CChat::JoinChannel");
	// Are we in a channel now?
	CChatChannel * pCurrentChannel = pMember->GetChannel();
	if (pCurrentChannel)
	{
		// Is it the same channel as the one I'm already in?
		if (strcmp(pszChannel, pCurrentChannel->GetName()) == 0)
		{
			// Tell them and return
			pMember->SendChatMsg(CHATMSG_AlreadyInConference, pszChannel);
			return false;
		}
	}
	CChatChannel * pNewChannel = FindChannel(pszChannel);
	if (!pNewChannel)
	{
		pMember->GetClient()->addChatSystemMessage(CHATMSG_NoConference, pszChannel );
		return false;
	}
	// If there's a password, is it the correct one?
	if (strcmp(pNewChannel->GetPassword(), pszPassword) != 0)
	{
		pMember->GetClient()->addChatSystemMessage(CHATMSG_IncorrectPassword);
		return false;
	}
	// Leave the old channel 1st
	// Remove from old channel (if any)
	if (pCurrentChannel)
	{
		// Remove myself from the channels list of members
		pCurrentChannel->RemoveMember(pMember);

		// If noone is left, tell the chat system to delete it from memory
		if (pCurrentChannel->m_Members.GetCount() <= 0)
		{
			// Am I the last one here? Delete it from all other clients?
			DeleteChannel(pCurrentChannel);
		}

		// Since we left, clear all members from our client that might be in our list from the channel we just left
		pMember->GetClient()->addChatSystemMessage(CHATMSG_ClearMemberList);
	}
	// Now join a new channel
	// Add all the members of the channel to the clients list of channel participants
	pNewChannel->SendMembers(pMember);
	// Add ourself to the channels list of members
	if (!(pNewChannel->AddMember(pMember)))
		return false;
	// Set the channel name title bar
	pMember->GetClient()->addChatSystemMessage(CHATMSG_UpdateChannelBar, pszChannel);
	// Now send out my name to all clients in this channel
	pNewChannel->SendThisMember(pMember);
	return true;
}

////////////////////////////////////////////////////////////////////////////
// -CChatChannel

void CChatChannel::WhoIs(const char * pszBy, const char * pszMember)
{
	ADDTOCALLSTACK("CChatChannel::WhoIs");
	CChatChanMember * pBy = FindMember(pszBy);
	CChatChanMember * pMember = FindMember(pszMember);
	CChar * pChar = pMember? pMember->GetClient()->GetChar() : NULL;
	if (!pMember||!pChar)
	{
		pBy->SendChatMsg(CHATMSG_NoPlayer, pszMember);
	}
	else if (pMember->GetWhoIs())
	{
		pBy->SendChatMsg(CHATMSG_PlayerKnownAs, pszMember, pChar->GetName());
	}
	else
	{
		pBy->SendChatMsg(CHATMSG_PlayerIsAnonymous, pszMember);
	}
}

void CChatChannel::Emote(const char * pszBy, const char * pszMsg, CLanguageID lang )
{
	ADDTOCALLSTACK("CChatChannel::Emote");
	if (HasVoice(pszBy))
		Broadcast(CHATMSG_PlayerEmote, pszBy, pszMsg, lang );
	else
		FindMember(pszBy)->SendChatMsg(CHATMSG_RevokedSpeaking);
}

void CChatChannel::ToggleVoiceDefault(const char *  pszBy)
{
	ADDTOCALLSTACK("CChatChannel::ToggleVoiceDefault");
	if (!IsModerator(pszBy))
	{
		FindMember(pszBy)->SendChatMsg(CHATMSG_MustHaveOps);
		return;
	}
	if (GetVoiceDefault())
		Broadcast(CHATMSG_ModeratorsSpeakDefault, "", "");
	else
		Broadcast(CHATMSG_SpeakingByDefault, "", "");
	SetVoiceDefault(!GetVoiceDefault());
}

void CChatChannel::DisableVoiceDefault(const char *  pszBy)
{
	ADDTOCALLSTACK("CChatChannel::DisableVoiceDefault");
	if (GetVoiceDefault())
		ToggleVoiceDefault(pszBy);
}

void CChatChannel::EnableVoiceDefault(const char *  pszBy)
{
	ADDTOCALLSTACK("CChatChannel::EnableVoiceDefault");
	if (!GetVoiceDefault())
		ToggleVoiceDefault(pszBy);
}

void CChatChannel::SendPrivateMessage(CChatChanMember * pFrom, const char * pszTo, const char *  pszMsg)
{
	ADDTOCALLSTACK("CChatChannel::SendPrivateMessage");
	CChatChanMember * pTo = FindMember(pszTo);
	if (!pTo)
	{
		pFrom->SendChatMsg(CHATMSG_NoPlayer, pszTo);
		return;
	}
	if (!pTo->IsReceivingAllowed())
	{
		pFrom->SendChatMsg(CHATMSG_PlayerNotReceivingPrivate, pszTo);
		return;
	}
	// Can always send private messages to moderators (but only if they are receiving)
	bool fHasVoice = HasVoice(pFrom->GetChatName());
	if ( !fHasVoice && !IsModerator(pszTo))
	{
		pFrom->SendChatMsg(CHATMSG_RevokedSpeaking);
		return;
	}

	if (pTo->IsIgnoring(pFrom->GetChatName())) // See if ignoring you
	{
		pFrom->SendChatMsg(CHATMSG_PlayerIsIgnoring, pszTo);
		return;
	}

	CGString sName;
	g_Serv.m_Chats.DecorateName(sName, pFrom);
	// Echo to the sending client so they know the message went out
	pFrom->SendChatMsg(CHATMSG_PlayerPrivate, sName, pszMsg);
	// If the sending and receiving are different send it out to the receiver
	if (pTo != pFrom)
		pTo->SendChatMsg(CHATMSG_PlayerPrivate, sName, pszMsg);
}

void CChatChannel::RenameChannel(CChatChanMember * pBy, const char * pszName)
{
	ADDTOCALLSTACK("CChatChannel::RenameChannel");
	// Ask the chat system if the new name is ok
	if ( ! g_Serv.m_Chats.IsValidName( pszName, false ))
	{
		pBy->SendChatMsg(CHATMSG_InvalidConferenceName);
		return;
	}
	// Ask the chat system if the new name is already taken
	if ( g_Serv.m_Chats.IsDuplicateChannelName(pszName))
	{
		pBy->SendChatMsg(CHATMSG_AlreadyAConference);
		return;
	}
	// Tell the channel members our name changed
	Broadcast(CHATMSG_ConferenceRenamed, GetName(), pszName);
	// Delete the old name from all chat clients
	g_Serv.m_Chats.SendDeleteChannel(this);
	// Do the actual renaming
	SetName(pszName);
	// Update all channel members' current channel bar
	Broadcast(CHATMSG_UpdateChannelBar, pszName, "");
	// Send out the new name to all chat clients so they can join
	g_Serv.m_Chats.SendNewChannel(this);
}

void CChatChannel::KickAll(CChatChanMember * pMemberException)
{
	ADDTOCALLSTACK("CChatChannel::KickAll");
	for (int i = 0; i < m_Members.GetCount(); i++)
	{
		if ( m_Members[i] == pMemberException) // If it's not me, then kick them
			continue;
		KickMember( pMemberException, m_Members[i] );
	}
}

void CChatChannel::RemoveMember(CChatChanMember * pMember)
{
	ADDTOCALLSTACK("CChatChannel::RemoveMember");
	for ( int i = 0; i < m_Members.GetCount(); i++)
	{
		// Tell the other clients in this channel (if any) you are leaving (including yourself)
		CClient * pClient = m_Members[i]->GetClient();

		if ( !pClient )		//	auto-remove offline clients
		{
			m_Members[i]->SetChannel(NULL);
			m_Members.RemoveAt(i);
			i--;
		}
		else
		{
			pClient->addChatSystemMessage(CHATMSG_RemoveMember, pMember->GetChatName());
			if (m_Members[i] == pMember)	// disjoin
			{
				m_Members.RemoveAt(i);
				break;
			}
		}
	}

	// Update our persona
	pMember->SetChannel(NULL);
}

bool CChatChannel::IsModerator(const char * pszMember) const
{
	ADDTOCALLSTACK("CChatChannel::IsModerator");
	for(int i = 0; i < m_Moderators.GetCount(); i++)
	{
		if (m_Moderators[i]->Compare(pszMember) == 0)
			return true;
	}
	return false;
}

bool CChatChannel::HasVoice(const char * pszMember) const
{
	ADDTOCALLSTACK("CChatChannel::HasVoice");
	for(int i = 0; i < m_NoVoices.GetCount(); i++)
	{
		if (m_NoVoices[i]->Compare(pszMember) == 0)
			return false;
	}
	return true;
}

void CChatChannel::SetModerator(const char * pszMember, bool fFlag)
{
	ADDTOCALLSTACK("CChatChannel::SetModerator");
	// See if they are already a moderator
	for(int i = 0; i < m_Moderators.GetCount(); i++)
	{
		if (m_Moderators[i]->Compare(pszMember) == 0)
		{
			if (fFlag == false)
			{
				m_Moderators.DeleteAt(i);
				return;
			}
			else
			{
				return;
			}
		}
	}
	if (fFlag)
	{
		m_Moderators.Add( new CGString(pszMember) );
	}
}

void CChatChannel::KickMember(CChatChanMember *pByMember, CChatChanMember * pMember )
{
	ADDTOCALLSTACK("CChatChannel::KickMember");
	ASSERT( pMember );

	LPCTSTR pszByName;
	if (pByMember) // If NULL, then an ADMIN or a GM did it
	{
		pszByName = pByMember->GetChatName();
		if (!IsModerator(pszByName))
		{
			pByMember->SendChatMsg(CHATMSG_MustHaveOps);
			return;
		}
	}
	else
	{
		pszByName = "SYSTEM";
	}

	LPCTSTR pszName = pMember->GetChatName();

	// Kicking this person...remove from list of moderators first
	if (IsModerator(pszName))
	{
		SetModerator(pszName, false);
		SendThisMember(pMember);
		Broadcast(CHATMSG_PlayerNoLongerModerator, pszName, "");
		pMember->SendChatMsg(CHATMSG_RemovedListModerators, pszByName);
	}

	// Now kick them
	if (m_Members.GetCount() == 1) // If kicking yourself, send out to all clients in a chat that the channel is gone
		g_Serv.m_Chats.SendDeleteChannel(this);
	// Remove them from the channels list of members
	RemoveMember(pMember);
	// Tell the remain members about this
	Broadcast(CHATMSG_PlayerIsKicked, pszName, "");
	// Now clear their channel member list
	pMember->SendChatMsg(CHATMSG_ClearMemberList);
	// And give them the bad news
	pMember->SendChatMsg(CHATMSG_ModeratorHasKicked, pszByName);
}

bool CChatChannel::AddMember(CChatChanMember * pMember)
{
	ADDTOCALLSTACK("CChatChannel::AddMember");
	pMember->SetChannel(this);
	m_Members.Add( pMember );
	// See if only moderators have a voice by default
	LPCTSTR pszName = pMember->GetChatName();
	if (!GetVoiceDefault() && !IsModerator(pszName))
		// If only moderators have a voice by default, then add this member to the list of no voices
		SetVoice(pszName, false);
	// Set voice status
	return true;
}

void CChatChannel::SendMembers(CChatChanMember * pMember)
{
	ADDTOCALLSTACK("CChatChannel::SendMembers");
	for (int i = 0; i < m_Members.GetCount(); i++)
	{
		CGString sName;
		g_Serv.m_Chats.DecorateName(sName, m_Members[i]);
		pMember->SendChatMsg(CHATMSG_SendPlayerName, sName);
	}
}

void CChatChannel::SendThisMember(CChatChanMember * pMember, CChatChanMember * pToMember)
{
	ADDTOCALLSTACK("CChatChannel::SendThisMember");
	char buffer[2048];
	sprintf(buffer, "%s%s",
		(IsModerator(pMember->GetChatName()) == true) ? "1" :
		(HasVoice(pMember->GetChatName()) == true) ? "0" : "2", pMember->GetChatName());
	// If no particular member is specified in pToMember, then send it out to all members
	if (pToMember == NULL)
	{
		for (int i = 0; i < m_Members.GetCount(); i++)
		{
			// Don't send out members if they are ignored by someone
			if (!m_Members[i]->IsIgnoring(pMember->GetChatName()))
				m_Members[i]->SendChatMsg(CHATMSG_SendPlayerName, buffer);
		}
	}
	else
	{
		// Don't send out members if they are ignored by someone
		if (!pToMember->IsIgnoring(pMember->GetChatName()))
			pToMember->SendChatMsg(CHATMSG_SendPlayerName, buffer);
	}
}

void CChatChannel::SetVoice(const char * pszName, bool fFlag)
{
	ADDTOCALLSTACK("CChatChannel::SetVoice");
	// See if they have no voice already
	for(int i = 0; i < m_NoVoices.GetCount(); i++)
	{
		if (m_NoVoices[i]->Compare(pszName) == 0)
		{
			if (fFlag == true)
			{
				m_NoVoices.DeleteAt(i);
				return;
			}
			else
				return;
		}
	}
	if (fFlag == false)
	{
		m_NoVoices.Add( new CGString(pszName) );
		return;
	}
}

void CChatChannel::MemberTalk(CChatChanMember * pByMember, const char * pszText, CLanguageID lang )
{
	ADDTOCALLSTACK("CChatChannel::MemberTalk");
	// Do I have a voice?
	if (!HasVoice(pByMember->GetChatName()))
	{
		pByMember->SendChatMsg(CHATMSG_RevokedSpeaking);
		return;
	}
	Broadcast(CHATMSG_PlayerTalk, pByMember->GetChatName(), pszText, lang );
}

void CChatChannel::ChangePassword(CChatChanMember * pByMember, const char * pszPassword)
{
	ADDTOCALLSTACK("CChatChannel::ChangePassword");
	if (!IsModerator(pByMember->GetChatName()))
	{
		pByMember->GetClient()->addChatSystemMessage(CHATMSG_MustHaveOps);
	}
	else
	{
		SetPassword(pszPassword);
		g_Serv.m_Chats.SendNewChannel(pByMember->GetChannel());
		Broadcast(CHATMSG_PasswordChanged, "","");
	}
}

void CChatChannel::Broadcast(CHATMSG_TYPE iType, const char * pszName, const char * pszText, CLanguageID lang, bool fOverride )
{
	ADDTOCALLSTACK("CChatChannel::Broadcast");
	CGString sName;
	CChatChanMember *pSendingMember = FindMember(pszName);

	if (iType >= CHATMSG_PlayerTalk && iType <= CHATMSG_PlayerPrivate) // Only chat, emote, and privates get a color status number
		g_Serv.m_Chats.DecorateName(sName, pSendingMember, fOverride);
	else
		sName = pszName;

	for (int i = 0; i < m_Members.GetCount(); i++)
	{
		// Check to see if the recipient is ignoring messages from the sender
		// Just pass over it if it's a regular talk message
		if (!m_Members[i]->IsIgnoring(pszName))
		{
			m_Members[i]->SendChatMsg(iType, sName, pszText, lang );
		}

		// If it's a private message, then tell the sender the recipient is ignoring them
		else if (iType == CHATMSG_PlayerPrivate)
		{
			pSendingMember->SendChatMsg(CHATMSG_PlayerIsIgnoring, m_Members[i]->GetChatName());
		}
	}
}

void CChatChannel::GrantVoice(CChatChanMember * pByMember, const char * pszName)
{
	ADDTOCALLSTACK("CChatChannel::GrantVoice");
	if (!IsModerator(pByMember->GetChatName()))
	{
		pByMember->SendChatMsg(CHATMSG_MustHaveOps);
		return;
	}
	CChatChanMember * pMember = FindMember(pszName);
	if (!pMember)
	{
		pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
		return;
	}
	if (HasVoice(pszName))
		return;
	SetVoice(pszName, true);
	SendThisMember(pMember); // Update the color
	pMember->SendChatMsg(CHATMSG_ModeratorGrantSpeaking, pByMember->GetChatName());
	Broadcast(CHATMSG_PlayerNowSpeaking, pszName, "", "");
}

void CChatChannel::RevokeVoice(CChatChanMember * pByMember, const char * pszName)
{
	ADDTOCALLSTACK("CChatChannel::RevokeVoice");
	if (!IsModerator(pByMember->GetChatName()))
	{
		pByMember->SendChatMsg(CHATMSG_MustHaveOps);
		return;
	}
	CChatChanMember * pMember = FindMember(pszName);
	if (!pMember)
	{
		pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
		return;
	}
	if (!HasVoice(pszName))
		return;
	SetVoice(pszName, false);
	SendThisMember(pMember); // Update the color
	pMember->SendChatMsg(CHATMSG_ModeratorRemovedSpeaking, pByMember->GetChatName());
	Broadcast(CHATMSG_PlayerNoSpeaking, pszName, "", "");
}

void CChatChannel::ToggleVoice(CChatChanMember * pByMember, const char * pszName)
{
	ADDTOCALLSTACK("CChatChannel::ToggleVoice");
	if (!HasVoice(pszName)) // (This also returns true if this person is not in the channel)
		GrantVoice(pByMember, pszName); // this checks and reports on membership
	else
		RevokeVoice(pByMember, pszName); // this checks and reports on membership
}

int CChatChannel::FindMemberIndex(const char * pszName) const
{
	ADDTOCALLSTACK("CChatChannel::FindMemberIndex");
	for(int i = 0; i < m_Members.GetCount(); i++)
	{
		if ( strcmp( m_Members[i]->GetChatName(), pszName) == 0)
			return i;
	}
	return -1;
}

void CChatChannel::GrantModerator(CChatChanMember * pByMember, const char * pszName)
{
	ADDTOCALLSTACK("CChatChannel::GrantModerator");
	if (!IsModerator(pByMember->GetChatName()))
	{
		pByMember->SendChatMsg(CHATMSG_MustHaveOps);
		return;
	}
	CChatChanMember * pMember = FindMember(pszName);
	if (!pMember)
	{
		pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
		return;
	}
	if (IsModerator(pMember->GetChatName()))
		return;
	SetModerator(pszName, true);
	SendThisMember(pMember); // Update the color
	Broadcast(CHATMSG_PlayerIsAModerator, pMember->GetChatName(), "", "");
	pMember->SendChatMsg(CHATMSG_YouAreAModerator, pByMember->GetChatName());
}

void CChatChannel::RevokeModerator(CChatChanMember * pByMember, const char * pszName)
{
	ADDTOCALLSTACK("CChatChannel::RevokeModerator");
	if (!IsModerator(pByMember->GetChatName()))
	{
		pByMember->SendChatMsg(CHATMSG_MustHaveOps);
		return;
	}
	CChatChanMember * pMember = FindMember(pszName);
	if (!pMember)
	{
		pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
		return;
	}
	if (!IsModerator(pMember->GetChatName()))
		return;
	SetModerator(pszName, false);
	SendThisMember(pMember); // Update the color
	Broadcast(CHATMSG_PlayerNoLongerModerator, pMember->GetChatName(), "", "");
	pMember->SendChatMsg(CHATMSG_RemovedListModerators, pByMember->GetChatName());
}

void CChatChannel::ToggleModerator(CChatChanMember * pByMember, const char * pszName)
{
	ADDTOCALLSTACK("CChatChannel::ToggleModerator");
	if (!IsModerator(pszName))
		GrantModerator(pByMember, pszName);
	else
		RevokeModerator(pByMember, pszName);
}

////////////////////////////////////////////////////////////////////////////
// -CChatChanMember

CChatChanMember::~CChatChanMember()
{
	if ( IsChatActive()) // Are we chatting currently ?
	{
		g_Serv.m_Chats.QuitChat(this);
	}
}

void CChatChanMember::SetChatActive()
{
	ADDTOCALLSTACK("CChatChanMember::SetChatActive");
	// called from Event_ChatButton
	if ( IsChatActive() )
		return;

	CClient * pClient = GetClient();
	if ( pClient )
	{
		m_fChatActive = true;

		// Tell the client to open the chat window dialog
		pClient->addChatSystemMessage( CHATMSG_OpenChatWindow, GetChatName() );

		// Send all existing channel names to this client
		const CChatChannel *pChannel = g_Serv.m_Chats.GetFirstChannel();
		for ( ; pChannel != NULL; pChannel = pChannel->GetNext() )
		{
			pClient->addChatSystemMessage(CHATMSG_SendChannelName, pChannel->GetName(), pChannel->GetModeString());
		}
	}
}

void CChatChanMember::SetChatInactive()
{
	ADDTOCALLSTACK("CChatChanMember::SetChatInactive");
	m_fChatActive = false;
}

int CChatChanMember::FindIgnoringIndex(LPCTSTR pszName) const
{
	ADDTOCALLSTACK("CChatChanMember::FindIgnoringIndex");
	for ( int i = 0; i < m_IgnoredMembers.GetCount(); i++)
	{
		if (m_IgnoredMembers[i]->Compare(pszName) == 0)
			return i;
	}
	return -1;
}

void CChatChanMember::Ignore(const char * pszName)
{
	ADDTOCALLSTACK("CChatChanMember::Ignore");
	if (!IsIgnoring(pszName))
		ToggleIgnore(pszName);
	else
		SendChatMsg(CHATMSG_AlreadyIgnoringPlayer, pszName);
}

void CChatChanMember::DontIgnore(const char * pszName)
{
	ADDTOCALLSTACK("CChatChanMember::DontIgnore");
	if (IsIgnoring(pszName))
		ToggleIgnore(pszName);
	else
		SendChatMsg(CHATMSG_NotIgnoring, pszName);
}

void CChatChanMember::ToggleIgnore(const char * pszName)
{
	ADDTOCALLSTACK("CChatChanMember::ToggleIgnore");
	int i = FindIgnoringIndex( pszName );
	if ( i>=0 )
	{
		m_IgnoredMembers.DeleteAt(i);

		SendChatMsg(CHATMSG_NoLongerIgnoring, pszName);

		// Resend the un ignored member to the client's local list of members (but only if they are currently in the same channel!)
		if (m_pChannel)
		{
			CChatChanMember * pMember = m_pChannel->FindMember(pszName);
			if (pMember)
				m_pChannel->SendThisMember(pMember, this);
		}
	}
	else
	{
		CGString * name = new CGString(pszName);
		m_IgnoredMembers.Add( name );
		SendChatMsg(CHATMSG_NowIgnoring, pszName); // This message also takes the ignored person off the clients local list of channel members
	}
}

void CChatChanMember::ClearIgnoreList()
{
	ADDTOCALLSTACK("CChatChanMember::ClearIgnoreList");
	for(int i = 0; i < m_IgnoredMembers.GetCount(); i++)
	{
		m_IgnoredMembers.DeleteAt(i);
	}
	SendChatMsg(CHATMSG_NoLongerIgnoringAnyone);
}

void CChatChanMember::RenameChannel(const char * pszName)
{
	ADDTOCALLSTACK("CChatChanMember::RenameChannel");
	CChatChannel * pChannel = GetChannel();
	if (!pChannel)
		SendChatMsg(CHATMSG_MustBeInAConference);
	else if (!pChannel->IsModerator(pszName))
		SendChatMsg(CHATMSG_MustHaveOps);
	else
		pChannel->RenameChannel(this, pszName);
}

void CChatChanMember::SendChatMsg(CHATMSG_TYPE iType, LPCTSTR pszName1, LPCTSTR pszName2, CLanguageID lang )
{
	ADDTOCALLSTACK("CChatChanMember::SendChatMsg");
	GetClient()->addChatSystemMessage(iType, pszName1, pszName2, lang );
}

void CChatChanMember::ToggleReceiving()
{
	ADDTOCALLSTACK("CChatChanMember::ToggleReceiving");
	m_fReceiving = !m_fReceiving;
	GetClient()->addChatSystemMessage((m_fReceiving) ? CHATMSG_ReceivingPrivate : CHATMSG_NoLongerReceivingPrivate);
}

void CChatChanMember::PermitWhoIs()
{
	ADDTOCALLSTACK("CChatChanMember::PermitWhoIs");
	if (GetWhoIs())
		return;
	SetWhoIs(true);
	SendChatMsg(CHATMSG_ShowingName);
}

void CChatChanMember::ForbidWhoIs()
{
	ADDTOCALLSTACK("CChatChanMember::ForbidWhoIs");
	if (!GetWhoIs())
		return;
	SetWhoIs(false);
	SendChatMsg(CHATMSG_NotShowingName);
}

void CChatChanMember::ToggleWhoIs()
{
	ADDTOCALLSTACK("CChatChanMember::ToggleWhoIs");
	SetWhoIs(!GetWhoIs());
	SendChatMsg((GetWhoIs() == true) ? CHATMSG_ShowingName : CHATMSG_NotShowingName);
}

CClient * CChatChanMember::GetClient()
{
	ADDTOCALLSTACK("CChatChanMember::GetClient");
	return( static_cast <CClient*>( this ));
}

const CClient * CChatChanMember::GetClient() const
{
	ADDTOCALLSTACK("CChatChanMember::GetClient");
	return( static_cast <const CClient*>( this ));
}

LPCTSTR CChatChanMember::GetChatName() const
{
	ADDTOCALLSTACK("CChatChanMember::GetChatName");
	return( GetClient()->GetAccount()->m_sChatName );
}
