with open('src/UIManager.h', 'r') as f:
    content = f.read()

content = content.replace("<<<<<<< HEAD", "")
content = content.replace("=======", "")
content = content.replace(">>>>>>> origin/master", "")

with open('src/UIManager.h', 'w') as f:
    f.write(content)
