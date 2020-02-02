class TestCommand : ICommand
{
	private int a;
	private int b;

	TestCommand(int a, int b)
	{
		Log("Instantiating TestCommand.\n");
		this.a = a;
		this.b = b;
	}

	int opCall()
	{
		game_server.SendToAll("Ran TestCommand");
		return 0;
	}
};
class Meower
{
	Meower()
	{
		Log("Trying to make a meower.\n");
		GenerateUUID(m_uuid);
		Log("Meower uuid: " + m_uuid + "\n");
	}
	string GetUUID()
	{
		return m_uuid;
	}
	private string m_uuid;
};

enum PlayerGameState {LOGIN_MENU = 0,
ACCOUNT_NAME_ENTRY, ACCOUNT_PASSWORD_ENTRY };
class Player : PlayerConnection
{

	Player()
	{
		super();
		m_gamestate = PlayerGameState::LOGIN_MENU;
	}

	PlayerGameState GetPlayerGameState()
	{
		return m_gamestate;
	}

	void SetPlayerGameState(PlayerGameState v)
	{
		m_gamestate = v;
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

		Log("Attempting to set player meower.\n");
		Send("Trying to set your meower.\r\n");
		string msg = "Your meower's UUID is " + m_meower.GetUUID() + "\r\n";
		Send(msg);
	}

	private PlayerGameState m_gamestate;
	private string m_name;
	private Meower@ m_meower;
};

TestCommand tc(1, 2);
Player@ hPlayer = null;
array<Player@> g_players;
array<Meower@> g_meowers;
void GameTick()
{
/*
  game_server.SendToAll("`red`Hello!`default`");
  game_server.QueueScriptCommand(tc, 4);
  game_server.QueueScriptCommand(tc, 2);
  if(hPlayer !is null)
  {
  hPlayer.Send("Ticking for player\r\n");
  }
  else
  {
  Log("There are no players connected.\r\n");
  }
*/
}



void OnPlayerConnect(Player@ player)
{
//	try
	{
	player.Send("Hello!\r\n");
	player.Send("Account: ");
	string uuid;
	GenerateUUID(uuid);
	player.Send("\r\n" + uuid + "\r\n");
	g_meowers.insertLast(Meower());
	player.SetMeower(g_meowers[g_meowers.length() - 1]);
	g_players.insertLast(player);
	game_server.SendToAll("Someone has connected. There are " + g_players.length() + " players connected.\r\n");
	}
//	catch
//	{
//		player.Send("Exception thrown!\r\n" + getExceptionInfo() + "\r\n");
//	}
/*
  Log("OnPlayerConnect()");
  player.Send("WELCOME!\r\n");
  @hPlayer = @player;
  if(hPlayer !is null)
  {
  hPlayer.Send("Handle assigned.\r\n");
  }
*/
}

void OnPlayerDisconnect(Player@ player)
{
	Log("OnPlayerDisconnect called.\n");
	int removeidx = g_players.findByRef(player);
	if(removeidx >= 0)
	{
		Log("Attempting to remove player at idx " + removeidx);
		int meoweridx = g_meowers.findByRef(player.GetMeower());
		game_server.SendToAll("g_meowers size: " + g_meowers.length() + "\r\n");
		g_meowers.removeAt(meoweridx);
		game_server.SendToAll("g_meowers size after removal: " + g_meowers.length() + "\r\n");
		g_players.removeAt(removeidx);
		game_server.SendToAll("Someone has disconnected. There are now " + g_players.length() + " players connected.\r\n");
	}
	else
	{
		Log("Couldn't find player in g_players.\n");
	}
/*
  Log("Someone disconnected.\r\n");
  if(@hPlayer is @player)
  {
  @hPlayer = null;
  }
*/
}

void OnPlayerInput(Player@ player, string rawinput)
{
	string input;
	TrimString(rawinput, input);
	if("makeuuid" == rawinput)
	{
		string uuid;
		GenerateUUID(uuid);
		player.Send("Generated: " + uuid + "\r\n");
	}
	else if("debugvars" == rawinput)
	{
		game_server.DebugVariables(player);
	}
	else
	{
		switch(player.GetPlayerGameState())
		{
		case PlayerGameState::LOGIN_MENU:
		{
			break;
		}
		case PlayerGameState::ACCOUNT_NAME_ENTRY:
			player.SetName(input);
			player.SetPlayerGameState(PlayerGameState::ACCOUNT_PASSWORD_ENTRY);
			break;
		case PlayerGameState::ACCOUNT_PASSWORD_ENTRY:
		{
			game_server.SendToAll(player.GetName() + " says: " + input + "\r\n");
			string output;
			HashPassword(input, output);
			player.Send("Your password hash is:" + output);
			break;
		}
		default:
			break;
		}
	}
}