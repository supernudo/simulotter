
-- Eurobot 2009: Atlantis

-- Colors
function color_i2f(r,g,b)
  return {r/255.0, g/255.0, b/255.0, 1.0}
end

colors = {
  white = {1.0, 1.0, 1.0, 1.0},
  black = {0.0, 0.0, 0.0, 1.0},
  plexi = {.70, .90, .95, 0.5},
  ral_6018 = color_i2f(0x4f, 0xa8, 0x33),
  ral_3020 = color_i2f(0xc7, 0x17, 0x12),
  ral_5015 = color_i2f(0x17, 0x61, 0xab),
  ral_8017 = color_i2f(0x2e, 0x1c, 0x1c),
}



-- Match

match = Match( { colors.ral_6018, colors.ral_3020 } )

function match:init(fconf)

  -- Field configuration
  --   columns: 1 to 10
  --   dispensers: (1 or 2) * 16
  local conf_col, conf_disp
  if fonc == nil or fconf < 0 then
    conf_col  = math.random(10)
    conf_disp = math.random(2)
  else
    conf_col  = fconf % 10 + 1
    conf_disp = math.floor(fconf/16) + 1
  end
  if conf_col < 1 or conf_col > 10 or conf_disp < 1 or conf_disp > 2 then
    error("invalid field configuration")
  end
  trace("Atlantis rules, columns: "..tostring(conf_col)..", dispensers: "..tostring(conf_disp))


  -- Various variables

  local c1, c2
  c1 = self:get_color(0)
  c2 = self:get_color(1)

  local table_size_x = 3.0
  local table_size_y = 2.1

  local col_space_x = 0.250
  local col_space_y = 0.200
  local col_offset_x = 0.400
  local col_offset_y = 0.125

  local disp_offset_x = 0.289
  local disp_offset_y = 0.250
  local disp_offset_z = 0.045

  local wall_width  = 0.022
  local wall_height = 0.070


  -- Ground
  trace("  ground")
  
  ground = OGround(colors.ral_5015, c1, c2)

  
  -- Walls (N, E, W, small SE, small SW, plexi S)
  trace(" walls")

  o = ObjectColor()
  o:add_geom(Geom:box(table_size_x+2*wall_width, wall_width, wall_height))
  o:init()
  o:set_pos(0, table_size_y/2+wall_width/2, wall_height/2)
  o:set_color(colors.white)
  o = ObjectColor()
  o:add_geom(Geom:box(wall_width, table_size_y+2*wall_width, wall_height))
  o:init();
  o:set_pos(table_size_x/2+wall_width/2, 0, wall_height/2)
  o:set_color(colors.white)
  o = ObjectColor()
  o:add_geom(Geom:box(wall_width, table_size_y+2*wall_width, wall_height))
  o:init()
  o:set_pos(-table_size_x/2-wall_width/2, 0, wall_height/2)
  o:set_color(colors.white)

  o = ObjectColor()
  o:add_geom(Geom:box(wall_width, 0.100, wall_height))
  o:init()
  o:set_pos(0.900+wall_width/2, -table_size_y/2+0.050, wall_height/2)
  o:set_color(colors.white)
  o = ObjectColor()
  o:add_geom(Geom:box(wall_width, 0.100, wall_height))
  o:init()
  o:set_pos(-0.900-wall_width/2, -table_size_y/2+0.050, wall_height/2)
  o:set_color(colors.white)

  o = ObjectColor()
  o:add_geom(Geom:box(1.800+wall_width, config.draw_epsilon, 0.250))
  o:init()
  o:set_pos(0, -table_size_y/2, 0.125)
  o:set_color(colors.plexi)
  o = ObjectColor()
  o:add_geom(Geom:box(0.578+wall_width, config.draw_epsilon, wall_height))
  o:init()
  o:set_pos(1.200, -table_size_y/2, wall_height/2)
  o:set_color(colors.plexi)
  o = ObjectColor()
  o:add_geom(Geom:box(0.578+wall_width, config.draw_epsilon, wall_height))
  o:init()
  o:set_pos(-1.200, -table_size_y/2, wall_height/2)
  o:set_color(colors.plexi)


  -- Building areas
  trace("  building areas")

  o = ObjectColor()
  o:add_geom(Geom:box(1.800, 0.100, 2*config.draw_epsilon))
  o:init()
  o:set_pos(0, 0.050-table_size_y/2, config.draw_epsilon)
  o:set_color(colors.ral_8017)

  o = ObjectColor()
  o:add_geom(Geom:box(0.600, 0.100, 0.030))
  o:init()
  o:set_pos(0, 0.050-table_size_y/2, 0.015)
  o:set_color(colors.ral_8017)

  o = ObjectColor()
  o:add_geom(Geom:cylinder(0.150, 0.060))
  o:init()
  o:set_pos(0, 0, 0.030)
  o:set_color(colors.ral_8017)


  -- Random column elements
  trace("  random columns elements")

  local col_placements = {
    {0,1,2}, {0,2,5}, {0,2,4}, {0,2,3}, {0,1,4},
    {0,1,5}, {0,4,5}, {0,1,3}, {0,3,5}, {0,3,4}
  }

  for i,j in ipairs(col_placements[conf_col]) do
    -- First team
    o = OColElem()
    o:set_color(c1)
    o:set_pos(-col_offset_x-(2-math.floor(j%3))*col_space_x, -col_offset_y+(3-math.floor(j/3))*col_space_y)
    o = OColElem()
    o:set_color(c1)
    o:set_pos(-col_offset_x-(2-math.floor(j%3))*col_space_x, -col_offset_y+math.floor(j/3)*col_space_y)
    -- Second team
    o = OColElem()
    o:set_color(c2)
    o:set_pos(col_offset_x+(2-math.floor(j%3))*col_space_x, -col_offset_y+(3-math.floor(j/3))*col_space_y)
    o = OColElem()
    o:set_color(c2)
    o:set_pos(col_offset_x+(2-math.floor(j%3))*col_space_x, -col_offset_y+math.floor(j/3)*col_space_y)
  end


  -- Dispensers
  trace("  dispensers")

  -- Fixed
  od = ODispenser()
  od:set_pos(table_size_x/2-disp_offset_x, -table_size_y/2, disp_offset_z, 2)
  for i = 0,1 do
    o = OColElem()
    o:set_color(c1)
    od:fill(o, i*0.035)
  end
  od = ODispenser()
  od:set_pos(disp_offset_x-table_size_x/2, -table_size_y/2, disp_offset_z, 2)
  for i = 0,1 do
    o = OColElem()
    o:set_color(c2)
    od:fill(o, i*0.035)
  end

  -- Random
  od = ODispenser()
  od:set_pos(table_size_x/2-wall_width/2, conf_disp==0 and disp_offset_y or -disp_offset_y, disp_offset_z, 1)
  for i = 0,1 do
    o = OColElem()
    o:set_color(c1)
    od:fill(o, i*0.035)
  end
  od = ODispenser()
  od:set_pos(-table_size_x/2+wall_width/2, conf_disp==0 and disp_offset_y or -disp_offset_y, disp_offset_z, 3)
  for i = 0,1 do
    o = OColElem()
    o:set_color(c2)
    od:fill(o, i*0.035)
  end


  -- Lintels and lintel storages
  trace("  lintels and lintel storages")

  ols = OLintelStorage()
  ols:set_pos(-0.200, 0)
  ol = OLintel()
  ol:set_color(c1)
  ols:fill(ol)

  ols = OLintelStorage()
  ols:set_pos(-0.600, 0)
  ol = OLintel()
  ol:set_color(c1)
  ols:fill(ol)

  ols = OLintelStorage()
  ols:set_pos(0.200, 0)
  ol = OLintel()
  ol:set_color(c2)
  ols:fill(ol)

  ols = OLintelStorage()
  ols:set_pos(0.600, 0)
  ol = OLintel()
  ol:set_color(c2)
  ols:fill(ol)
end


