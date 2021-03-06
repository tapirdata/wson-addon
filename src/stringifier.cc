#include "stringifier.h"

Stringifier::Stringifier(v8::Local<v8::Function> errorClass, v8::Local<v8::Object> options): st_(*this) {
  const v8::Local<v8::Context> context = Nan::GetCurrentContext();
  errorClass_.Reset(errorClass);
  v8::Local<v8::Value> conDefsValue = options->Get(context, Nan::New("connectors").ToLocalChecked()).ToLocalChecked();
  if (conDefsValue->IsObject()) {
    v8::Local<v8::Object> conDefs = conDefsValue.As<v8::Object>();
    v8::Local<v8::Array> names = conDefs->GetOwnPropertyNames(context).ToLocalChecked();
    uint32_t len = names->Length();
    connectors_.resize(len);
    for (uint32_t i=0; i<len; ++i) {
      v8::Local<v8::String> name = names->Get(context, i).ToLocalChecked().As<v8::String>();
      v8::Local<v8::Object> conDef = conDefs->Get(context, name).ToLocalChecked().As<v8::Object>();
      StringifyConnector* connector = new StringifyConnector();
      connector->self.Reset(conDef);
      connector->by.Reset(
        conDef->Get(context, Nan::New(sBy)).ToLocalChecked().As<v8::Function>()
      );
      connector->split.Reset(
        conDef->Get(context, Nan::New(sSplit)).ToLocalChecked().As<v8::Function>()
      );
      connector->name.appendHandleEscaped(name);
      connectors_[i] = connector;
    }
  }
  // std::cout << "Stringifier connectors_.size()=" << connectors_.size() << std::endl;
};

Stringifier::~Stringifier() {
  errorClass_.Reset();
  for (ConnectorVector::iterator it=connectors_.begin(); it != connectors_.end(); ++it) {
    delete (*it);
  }
  connectors_.clear();
};

Nan::Persistent<v8::Function> Stringifier::constructor;
Nan::Persistent<v8::String> Stringifier::sBy;
Nan::Persistent<v8::String> Stringifier::sSplit;
Nan::Persistent<v8::String> Stringifier::sConstructor;
Nan::Persistent<v8::Function> Stringifier::objectConstructor;

NAN_METHOD(Stringifier::New) {
  Nan::HandleScope();
  if (info.Length() < 1 || !(info[0]->IsFunction())) {
    return Nan::ThrowTypeError("First argument should be an error constructor");
  }
  if (info.Length() < 2 || !(info[0]->IsObject())) {
    return Nan::ThrowTypeError("Second argument should be an option object");
  }
  v8::Local<v8::Function> errorClass = info[0].As<v8::Function>();
  v8::Local<v8::Object> options = info[1].As<v8::Object>();
  if (info.IsConstructCall()) {
    Stringifier* obj = new Stringifier(errorClass, options);
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    const int argc = 2;
    v8::Local<v8::Value> argv[argc] = {errorClass, options};
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    // info.GetReturnValue().Set(cons->NewInstance(argc, argv));
    Nan::MaybeLocal<v8::Object> result = Nan::NewInstance(cons, argc, argv);
    if (!result.IsEmpty()) {
      info.GetReturnValue().Set(result.ToLocalChecked());
    }
  }
}

NAN_METHOD(Stringifier::Escape) {
  Nan::HandleScope();
  TargetBuffer target;
  if (info.Length() < 1 || !(info[0]->IsString())) {
    return Nan::ThrowTypeError("First argument should be a string");
  }
  v8::Local<v8::String> s = info[0].As<v8::String>();
  // Stringifier* self = node::ObjectWrap::Unwrap<Stringifier>(info.This());
  target.appendHandleEscaped(s);
  info.GetReturnValue().Set(target.getHandle());
}


NAN_METHOD(Stringifier::GetTypeid) {
  Nan::HandleScope();
  if (info.Length() < 1) {
    return Nan::ThrowTypeError("Missing argument");
  }
  int ti = getTypeid(info[0]);
  info.GetReturnValue().Set(Nan::New<v8::Number>(ti));
}


NAN_METHOD(Stringifier::Stringify) {
  Nan::HandleScope();
  Stringifier* self = node::ObjectWrap::Unwrap<Stringifier>(info.This());
  StringifierTarget &st = self->st_;
  if (info.Length() < 1) {
    return Nan::ThrowTypeError("Missing first argument");
  }

  Nan::Callback *haverefCb = NULL;
  if (info.Length() >= 2 && (info[1]->IsFunction())) {
    v8::Local<v8::Function> haveCbHandle = info[1].As<v8::Function>();
    haverefCb = new Nan::Callback(haveCbHandle);
  }

  st.clear(haverefCb);
  st.put(info[0]);

  v8::Local<v8::Value> result = st.target.getHandle();
  delete haverefCb;
  info.GetReturnValue().Set(result);
}

NAN_METHOD(Stringifier::ConnectorOfValue) {
  Nan::HandleScope();
  if (info.Length() < 1) {
    return Nan::ThrowTypeError("Missing first argument");
  }
  Stringifier* self = node::ObjectWrap::Unwrap<Stringifier>(info.This());
  const StringifyConnector* connector = NULL;
  if (info[0]->IsObject()) {
    connector = self->findConnector(info[0].As<v8::Object>());
  }
  v8::Local<v8::Value> result;
  if (connector) {
    result = Nan::New<v8::Object>(connector->self);
  } else {
    result = Nan::Null();
  }
  info.GetReturnValue().Set(result);
}

void Stringifier::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope();
  const v8::Local<v8::Context> context = Nan::GetCurrentContext();

  v8::Local<v8::FunctionTemplate> newTpl = Nan::New<v8::FunctionTemplate>(New);
  newTpl->SetClassName(Nan::New("Stringifier").ToLocalChecked());
  newTpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(newTpl, "escape", Escape);
  Nan::SetPrototypeMethod(newTpl, "getTypeid", GetTypeid);
  Nan::SetPrototypeMethod(newTpl, "stringify", Stringify);
  Nan::SetPrototypeMethod(newTpl, "connectorOfValue", ConnectorOfValue);

  constructor.Reset(newTpl->GetFunction(context).ToLocalChecked());
  sBy.Reset(Nan::New("by").ToLocalChecked());
  sSplit.Reset(Nan::New("split").ToLocalChecked());
  sConstructor.Reset(Nan::New("constructor").ToLocalChecked());
  objectConstructor.Reset(
    Nan::New<v8::Object>()->Get(context, Nan::New(sConstructor)).ToLocalChecked().As<v8::Function>()
  );

  exports->Set(context, Nan::New("Stringifier").ToLocalChecked(), newTpl->GetFunction(context).ToLocalChecked()).ToChecked();

  StringifierTarget::Init();
}


