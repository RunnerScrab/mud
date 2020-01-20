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

TestCommand tc(1, 2);
Player@ hPlayer = null;

void GameTick()
{

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
}



void OnPlayerConnect(Player@ player)
{
	Log("OnPlayerConnect()");
	player.Send("WELCOME!\r\n");
	@hPlayer = @player;
	if(hPlayer !is null)
	{
		hPlayer.Send("Handle assigned.\r\n");
	}
}

void OnPlayerDisconnect(Player@ player)
{
	Log("Someone disconnected.\r\n");
	if(@hPlayer is @player)
	{
		@hPlayer = null;
	}
}

void OnPlayerInput(Player@ player, string msg)
{
	Log("Received player input.\r\n");
	player.Send("You sent: " + msg + "\r\n");
}