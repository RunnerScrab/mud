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

enum PlayerGameState {ACCOUNT_NAME_ENTRY = 0, ACCOUNT_PASSWORD_ENTRY };
class Player : PlayerConnection
{

	Player()
	{
		super();
		m_gamestate = PlayerGameState::ACCOUNT_NAME_ENTRY;
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

	private PlayerGameState m_gamestate;
	private string m_name;
};

TestCommand tc(1, 2);
Player@ hPlayer = null;

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
	player.Send("Account: ");
	string uuid;
	GenerateUUID(uuid);
	player.Send("\r\n" + uuid + "\r\n");
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
	else
	{
		switch(player.GetPlayerGameState())
		{
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
