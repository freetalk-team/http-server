// Base class for components
class ComponentBase {


	registerClickEvents(container) {

		container.onclick = (e) => {

			const target = e.target;

			switch (target.tagName) {

				case 'BUTTON':
				this.handleAction(target);
				break;

				default:
				this.handleClick(target);
			}
		}
	}

	handleAction() {}
	handleClick() {}
}

export class ListComponent extends ComponentBase {

	get classname() { return this.constructor.name; }

	constructor(container, ds) {

		super();
		
		this.container = typeof container == 'string'
			? dom.renderTemplate(container, {})
			: container;

		this.registerClickEvents(this.container);
		this.ds = ds;

		let e = this.container;
		
		if (!e.hasAttribute('list'))
			e = this.container.querySelector('[list]');

		if (e) {
			const template = e.getAttribute('list');

			this.list = UX.List.wrapGroup(e, template);

			this.#load();
		}
	}

	handleAction(target) {

		const item = target.closest('[item]');

		switch (target.name) {
			case 'rm':
			if (item) dom.removeElement(item);
			break;
		}

		this.onAction(target.name, item);
	}

	handleClick(target) {
		this.onClick(target);
	}

	// virtual
	load() {
		return this.ds ? this.ds.ls() : [];
	}

	onAction(name, item) {}

	// private
	async #load() {

		try {

			const items = await this.load();

			for (const i of items) 
				this.list.add(i);
		}
		catch (e) {
			console.error('Failed to load component:', this.classname, e);
		}

	}
}