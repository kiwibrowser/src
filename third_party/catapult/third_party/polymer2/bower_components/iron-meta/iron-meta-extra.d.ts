declare namespace Polymer {

  class IronMeta {
    constructor(options?: {
      type: string|null|undefined,
      key: string|null|undefined,
      value: any,
    });

    static types: {[type: string]: {[key: string]: any}};

    value: any;

    readonly list: any[]|undefined;

    byKey(key: string): any;
  }
}
