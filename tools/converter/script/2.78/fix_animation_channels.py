import bpy

print("Fixing animation channels...")
for action in bpy.data.actions:
	if len(action.groups) != 1:
		print("Animation '%s' already fixed." % action.name)
		continue
	baseGroup = action.groups[0]
	for channel in baseGroup.channels:
		name = channel.data_path.split('"')[1]
		if name not in action.groups:
			group = action.groups.new(name)
		else:
			group = action.groups[name]
		channel.group = group
	action.groups.remove(baseGroup)
	print("Fixed animation '%s'." % action.name)
print("Done")
