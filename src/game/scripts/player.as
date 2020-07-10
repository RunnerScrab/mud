class EditableString
{
	string text;

}

class Object
{
	string name;
	string sdesc;
	string mdesc;

	void EditObject(Player@ player, const string &in input)
	{

	}
}

class Player
{
	string m_name;
     	private Meower@ m_meower;

	weakref<PlayerConnection> m_connection;
	Character@ m_char;
	IPlayerInputMode@ currentmode;
	EditableText@ testeditstring;

	bool m_bEditMode;

	PlayerConnection@ GetConnection()
	{
		return m_connection.get();
	}

	Player(PlayerConnection@ conn)
	{
		@testeditstring = EditableText();
		testeditstring.SetMaxLines(2);
		@m_connection = conn;
		conn.SetInputCallback(InputCallback(OnInputReceived));
		conn.SetDisconnectCallback(DisconnectCallback(OnDisconnect));
		@m_char = Character("mychar");

		@currentmode = PlayerDefaultInputMode(this);
		//PlayerEditInputMode();
	}

	void OnInputReceived(string &in input)
	{
		if(currentmode !is null)
		{
			if("testedit" == input)
			{
				Send("Changing input mode.\r\n");
				@currentmode = PlayerEditInputMode(this, testeditstring);
			}
			else if("exitedit" == input)
			{
				Send("Edit complete.\r\n");
				Send(testeditstring.GetString() + "\r\n");
			}
			else
			{
				currentmode.OnInputReceived(input);
			}
		}
		else
		{
			Log("Current mode is null.\n");
		}
	}

	void Send(string input)
	{
		PlayerConnection@ conn = m_connection.get();
		if(conn !is null)
		{
			conn.Send(input);
		}
		else
		{
			Log("Tried to send to a closed connection.\n");
		}
	}

	void OnDisconnect()
	{
		Log("Callback: Player disconnecting\n");
		int removeidx = g_players.findByRef(this);
		if(removeidx >= 0)
		{
			Log("Attempting to remove player at idx " + removeidx);
			int meoweridx = g_meowers.findByRef(GetMeower());
			game_server.SendToAll("g_meowers size: " + g_meowers.length() + "\r\n");

			game_server.SendToAll("g_meowers size after removal: " + g_meowers.length() + "\r\n");
			g_players.removeAt(removeidx);
			game_server.SendToAll("Someone has disconnected. There are now " + g_players.length() + " players connected.\r\n");
			g_meowers.removeAt(meoweridx);
		}
		else
		{
			Log("Couldn't find player in g_players.\n");
		}


	}

	~Player()
	{
		Log("Calling script player destructor.\n");
//m_connection.DetachUserEventObserver(this);
	}

	string GetName()
	{
		return m_name;
	}

	void SetName(string v)
	{
		m_name = v;
	}

	Meower@ GetMeower() { return m_meower;}
	void SetMeower(Meower@ m)
	{
		@m_meower = @m;
		Log("Attempting to set player meower.\n");
		Send("Trying to set your meower. это - кошка. \r\n");
		string msg = "\r\nYour meower's UUID is `@ff0000`" + m_meower.GetUUID() + "`default`!\r\n";
		Send(msg);
	}


}