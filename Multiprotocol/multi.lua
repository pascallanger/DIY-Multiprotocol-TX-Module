--	Multiprotocole Midelic et Pascallanger
local debut = 0
local tps = 0
local tpsact = 1024
local mix, mixe
local channel

local inp = {
	{ "Protocole", VALUE, 1, 26, 2 },
	{ "Switch", SOURCE }
}
--	6	7	8			15	16	17			24	25	26
--	4		5			12	13	14			21	22	23
--	1	2	3			9	10	11			18	19	20
local out = { "Bind", "Gaz", "Aile", "Prof", "Dir" }

local function run_func(proto, sw)
	-- test mixage lua 
	if debut == 0 then
		-- passage en lua
		for channel = 0, 3, 1 do
			local mix = model.getMix(channel, 0)
			mix_source = mix["source"]
			if mix_source < 33 or 1 then 
				model.deleteMix(channel, 0)
				mix["source"] = channel + 34
				mix["name"] = "Lua "
				model.insertMix(channel, 0, mix)
			end
		end
	end
	-- inter install 
	channel = 4
	mix = { name="Raz Bind", source=33, weight=100, switch=0, multiplex=REPLACE }
	count = model.getMixesCount(channel + 0)
	if count == 0 and inter == 1 then
		model.insertMix(channel + 0, 0, mix)
	elseif count == 1 and inter == 0 then
		mixe = model.getMix(channel, 0)
		if mixe["name"] == mix["name"] then
			model.deleteMix(channel, 0)
		end
	end
	
	-- delais init
	if proto ~= debut then
		tps = getTime() + 500 	-- delai pour mini 12 cycle PPM
		tpsact = 1024
		debut = proto
	end
	
	local gaz = 1024
	local ail = 0
	local dir = 0
	local pro = 0
	
	if tpsact == 0 and sw < 200 then
		-- reprise valeur input
		pro = getValue(1)
		ail = getValue(2)
		gaz = getValue(3)
		dir = getValue(4)
	elseif tpsact ~= 0 then
		-- decallage pour position memo (centre)
		if proto > 4 then proto = proto + 1 end
		
		-- calcul position
		-- decallage pour > 18
		if proto > 18 then
			ail = 1024
			proto = proto - 18
		end
		-- decallage pour > 9
		if proto > 9 then
			ail = -1024
			proto = proto - 9
		end
		
		if proto < 4 then pro = -1024 end
		if proto > 6 then pro = 1024 end
		
		if proto % 3 == 1 then dir = -1024 end
		if proto % 3 == 0 then dir = 1024 end
		
		if tps < getTime() then tpsact = 0 end
		sw = tpsact
	end
	
	return sw, gaz, ail, pro, dir
end
return { run=run_func, input=inp, output=out}
