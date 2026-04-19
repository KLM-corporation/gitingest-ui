const path = require('path');

module.exports = {
  version: "1.0",
  title: "GitingestUI",
  description: "Transform any codebase into a single, LLM-ready text digest.",
<<<<<<< HEAD
  icon: "gitingest_logo.png",
=======
  icon: "src/gitingest_logo.png",
>>>>>>> 001fe176187e5cf6821e55be14f34d33914fc1d1
  menu: async (kernel) => {
    let installed = await kernel.exists(__dirname, "build", "Release", "GitingestUI.exe");
    let installing = await kernel.running(__dirname, "install.json");
    
    if (installing) {
      return [{
        default: true,
        icon: "fa-solid fa-spinner fa-spin",
        text: "Installing",
        href: "install.json"
      }];
    }
    
    if (installed) {
      let running = await kernel.running(__dirname, "start.json");
      if (running) {
        return [{
          default: true,
          icon: "fa-solid fa-spinner fa-spin",
          text: "Running",
          href: "start.json"
        }];
      } else {
        return [{
          default: true,
          icon: "fa-solid fa-power-off",
          text: "Start",
          href: "start.json"
        }, {
          icon: "fa-solid fa-plug",
          text: "Update",
          href: "update.json"
        }, {
<<<<<<< HEAD
          icon: "fa-solid fa-trash",
          text: "Uninstall",
          href: "uninstall.json"
=======
          icon: "fa-solid fa-plug",
          text: "Install",
          href: "install.json"
>>>>>>> 001fe176187e5cf6821e55be14f34d33914fc1d1
        }];
      }
    } else {
      return [{
        default: true,
        icon: "fa-solid fa-plug",
        text: "Install",
        href: "install.json"
      }];
    }
  }
};
