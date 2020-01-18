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
void GameTick()
{

	game_server.SendToAll("`red`Hello!`default`");
	game_server.QueueScriptCommand(tc, 4);
	game_server.QueueScriptCommand(tc, 2);
}
