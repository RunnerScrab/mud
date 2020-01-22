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

enum PlayerGameState {ACCOUNT_ENTRY = 0, INPUT };
class Player : PlayerConnection
{

	Player()
	{
		super();
		m_gamestate = PlayerGameState::ACCOUNT_ENTRY;
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

void OnPlayerInput(Player@ player, string input)
{
	switch(player.GetPlayerGameState())
	{
	case PlayerGameState::ACCOUNT_ENTRY:
		player.SetName(TrimString(input));
		player.SetPlayerGameState(PlayerGameState::INPUT);
		break;
	case PlayerGameState::INPUT:
		game_server.SendToAll(player.GetName() + " says: " + input + "\r\n");
		break;
	default:
		break;
	}
}
