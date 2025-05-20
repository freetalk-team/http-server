import { ListComponent } from './components/base.js';

class Datasource {
	get() {}
	ls() { return []; }
}

class DatasourceRest extends Datasource {
	constructor(path) {
		this.remote = path;
	}

	async get(id) {

		const res = await fetch(this.remote + '/' + id);
		if (!res.ok) {
			console.error(`Failed to load component: ${name}`);
			return null;
		}

		return res.json();
	}

	async ls(offset=0) {

		let path = this.remote;

		if (offset)
			path += '?o=' + offset;

		const res = await fetch(path);
		if (!res.ok) {
			console.error(`Failed to load component: ${name}`);
			return null;
		}

		return res.json();
	}
}

// Global App object
export default class App {
	static ListComponent = ListComponent;
	static DatasourceRest = DatasourceRest;

	static Components = {};

	datasource = {

		_local: {},
		_remote: {},

		add(name, ds) {

			if (this[name]) {
				console.error('Datasource already exists:', name);
				return;
			}

			if (typeof ds == 'object') {

				if (ds.remote)
					this[name] = new DatasourceRest(ds.remote);
			}

		}
	}

	static registerComponent(id, Class) {
		App.Components[id] = Class;

		if (Class.ds) {
			app.datasource.add(id, Class.ds);
		}

	}

	constructor() {
		window.app = this;
	}

	// Load and instantiate all components in the given container
	async loadComponents(root) {
		const components = root.querySelectorAll('component[name]');

		for (const el of components) {
			const name = el.getAttribute('name');

			let ComponentClass = App.Components[name];

			if (!ComponentClass) {
				// TODO: Load from IndexedDB or fetch
				const code = await this.loadComponent(name);

				if (!code) {
					console.warn(`Component "${name}" not found in DB`);
					continue;
				}

				// Parse and inject component code
				const container = document.createElement('div');
				container.innerHTML = code;

				// Append <template>
				const templates = container.querySelectorAll('template');
				const templateEl = templates[0];

				for (const i of templates)
					document.body.appendChild(i)

				// Append <style>
				const styleEl = container.querySelector('style');
				if (styleEl) document.head.appendChild(styleEl);

				// Evaluate <script> (sync)
				const scriptEl = container.querySelector('script');
				if (scriptEl) {
					const script = document.createElement('script');
					script.textContent = scriptEl.textContent;
					document.body.appendChild(script);
				}

				// Wait a tick to ensure class is registered
				await new Promise(res => setTimeout(res, 0));

				ComponentClass = App.Components[name];
				if (!ComponentClass) {
					console.error(`Failed to register component class for "${name}"`);
					continue;
				}

				// ComponentClass.id = name;
				ComponentClass.template = templateEl.id;
			}
		
			const comp = new ComponentClass(ComponentClass.template, this.datasource[name]);

			el.replaceWith(comp.container);


			// const templateEl = document.getElementById(templateId);

			// if (templateEl) {
			// 	const tmpl = templateEl.content.cloneNode(true);
			// 	const wrapper = document.createElement('div');
			// 	wrapper.appendChild(tmpl);
			// 	el.replaceWith(wrapper);
			// 	new ComponentClass(wrapper);
			// }
		}
	}

	async loadComponent(name) {

		const res = await fetch(`/components/${name}.xhtml`);
		if (!res.ok) {
			console.error(`Failed to load component: ${name}`);
			return null;
		}

		return res.text();

	}

	// Placeholder: Simulate load from IndexedDB
	async loadFromIndexedDB(name) {}
}
