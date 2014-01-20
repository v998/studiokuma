QQ_guf2hash=function(b,i)
	-- io.input('lua/qq/eqq.all.js')
	local str=OP_Get('http://0.web.qstatic.com/webqqpic/pubapps/0/50/eqq.all.js?t=20130723001',OP_referer_s) -- io.read('*all')
	print('QQ_guf2hash: str:len()='..str:len())

	local pos=str:find(')],b=',1,true)
	pos=pos+5
	local pos2=str:find('(',pos,true)
	local fn=str:sub(pos,pos2-1)
	local pos3;
	print('pos='..pos..' pos2='..pos2..' fn='..fn)

	pos=str:find(fn..'=function(',1,true)
	pos3=str:find('{',pos,true)+1
	while true do
		pos2=str:find('},',pos3,true)
		pos3=str:find('{',pos3,true)
		if pos3==nil or pos3>pos2 then break end
	end
	
	local func=str:sub(pos,pos2)
	print('\n'..func)

	local fh=io.open('lua/qq/tmp.js',"wb")
	fh:write(func)
	fh:write(',WScript.StdOut.WriteLine('..fn.."('"..b.."','"..i.."'))")
	fh:close()

	fh=io.open('lua/qq/eqq.all.js',"wb")
	fh:write(str)
	fh:close()

  local f = assert(io.popen('cscript lua\\qq\\tmp.js //Nologo', 'r'))
  local s = assert(f:read('*a'))
  f:close()
  
  s=s:match("^%s*(.-)%s*$")
  
  print('hash=|'..s..'|\n')
	
  return s
end -- QQ_guf2hash
