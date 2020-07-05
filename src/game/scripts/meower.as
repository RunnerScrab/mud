class TestPOD
{
	string m_name;
	string m_ability;
	TestPOD(string name, string ability)
	{
		m_name = name;
		m_ability = ability;
	}
};

class Meower : TestInterface, IPersistent
{
	uuid m_uuid;
	string m_name;
	array<TestPOD@> m_testpods;
	MPInt m_xval;
	MPFloat m_yval;

	void OnLoad(DBTable@ table, DBRow@ row)
	{
		row.GetColValue("uuid", m_uuid);
		row.GetColValue("name", m_name);
		row.GetColValue("xcoord", m_xval, 72);
		row.GetColValue("ycoord", m_yval, 6.28);

		array<DBRow@> resultarr;
		DBTable@ podsubtable = table.GetSubTable("testpodarray");
		if(@podsubtable !is null)
		{
			podsubtable.LoadSubTable(row, resultarr);
			for(int i = 0, len = resultarr.length(); i < len; ++i)
			{
				string name;
				string ability;
				DBRow@ hRow = resultarr[i];
				hRow.GetColValue("name", name);
				hRow.GetColValue("ability", ability);
				m_testpods.insertLast(TestPOD(name, ability));
			}
		}
	}

	void OnDefineSchema(DBTable@ table)
	{
		Log("Calling Meower's DefineSchema()\n");
		table.AddUUIDCol("uuid", DBKEYTYPE_PRIMARY);
		table.AddTextCol("name");
		table.AddMPIntCol("xcoord");
		table.AddMPFloatCol("ycoord");
		DBTable@ testpodtable = table.CreateSubTable("testpodarray");
		testpodtable.AddTextCol("name");
		testpodtable.AddTextCol("ability");
	}

	void OnSave(DBTable@ table, DBRow@ row)
	{
		Log("Calling Meower OnSave().\r\n");
		if(@row !is null)
		{
			row.SetColValue("uuid", m_uuid);
			row.SetColValue("name", m_name);
			row.SetColValue("xcoord", m_xval);
			row.SetColValue("ycoord", m_yval);

			DBTable@ testpodtable = table.GetSubTable("testpodarray");

			for(int i = 0, len = m_testpods.length();
			    i < len; ++i)
			{
//tptrow.ClearValues();
				TestPOD@ thispod = m_testpods[i];
				DBRow@ tptrow = DBRow(testpodtable);
				tptrow.SetColValue("subtable_index", i);
				tptrow.SetColValue("name", thispod.m_name);
				tptrow.SetColValue("ability", thispod.m_ability);
				row.StoreChildRow(tptrow);
			}

		}
		else
		{
			Log("OnSave was passed a null pointer.\n");
		}
	}

	void TestInterfaceMethod()
	{
		Log("Calling Meower's testinterface method.\n");
	}

	Meower()
	{
		Log("Meower constructing.\n");
		TestInterfaceMethod();
		m_name = "Meower";
		m_uuid.Generate();
		Log("Trying to make a meower. это - кошка!\n");
		Log("Meower uuid: " + m_uuid.ToString() + "\n");
		Log("Meower coord: " + m_xval.toString() + "\n");
		Log("Meower ycoord: " + m_yval.toString() + "\n");
	}
	~Meower()
	{

	}
	string GetUUID()
	{
		return m_uuid.ToString();
	}
};

class SuperMeower : Meower
{
	string m_superpowername;

	void TestInterfaceMethod()
	{
		Log("Calling SuperMeower's testinterface method.\n");
		Meower::TestInterfaceMethod();
	}

	void OnDefineSchema(DBTable@ table)
	{
		Meower::OnDefineSchema(table);
		Log("Calling SuperMeower's OnDefineSchema()\n");
		table.AddTextCol("superpower");
	}

	SuperMeower()
	{
		Log("SuperMeower constructing.\n");
		TestInterfaceMethod();
		m_superpowername = "meowing";
	}
	void OnSave(DBTable@ table, DBRow@ row)
	{
		Meower::OnSave(table, row);
		Log("Calling SuperMeower OnSave().\r\n");
		row.SetColValue("superpower", m_superpowername);
	}
	void OnLoad(DBTable@ table, DBRow@ row)
	{
		Meower::OnLoad(table, row);
		row.GetColValue("superpower", m_superpowername);
	}

};

class MegaMeower : SuperMeower
{
	MegaMeower()
	{

	}
	void OnDefineSchema(DBTable@ table)
	{
		SuperMeower::OnDefineSchema(table);
		Log("Calling MegaMeower's OnDefineSchema()");
	}

};
