/* class TestCommand : ICommand
{
	private int a;
	private int b;

	TestCommand(int a, int b)
	{
		this.a = a;
		this.b = b;
	}

	int opCall()
	{
		game_server.SendToAll("Ran TestCommand");
		return 0;
	}
};
*/
void GameTick()
{
	game_server.SendToAll("`red`Hello!`default`");
}
