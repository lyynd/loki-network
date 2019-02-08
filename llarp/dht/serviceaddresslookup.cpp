#include <dht/serviceaddresslookup.hpp>

#include <dht/context.hpp>
#include <dht/messages/findintro.hpp>
#include <dht/messages/gotintro.hpp>

namespace llarp
{
  namespace dht
  {
    ServiceAddressLookup::ServiceAddressLookup(
        const TXOwner &asker, const service::Address &addr,
        AbstractContext *ctx, uint64_t r,
        service::IntroSetLookupHandler handler)
        : TX< service::Address, service::IntroSet >(asker, addr, ctx)
        , handleResult(handler)
        , R(r)
    {
      peersAsked.insert(ctx->OurKey());
    }

    bool
    ServiceAddressLookup::Validate(const service::IntroSet &value) const
    {
      if(!value.Verify(parent->Crypto(), parent->Now()))
      {
        llarp::LogWarn("Got invalid introset from service lookup");
        return false;
      }
      if(value.A.Addr() != target)
      {
        llarp::LogWarn("got introset with wrong target from service lookup");
        return false;
      }
      return true;
    }

    bool
    ServiceAddressLookup::GetNextPeer(Key_t &next,
                                      const std::set< Key_t > &exclude)
    {
      Key_t k    = target.ToKey();
      auto nodes = parent->Nodes();
      if(nodes)
      {
        return nodes->FindCloseExcluding(k, next, exclude);
      }
      else
      {
        return false;
      }
    }

    void
    ServiceAddressLookup::Start(const TXOwner &peer)
    {
      parent->DHTSendTo(peer.node.as_array(),
                        new FindIntroMessage(peer.txid, target, R));
    }

    void
    ServiceAddressLookup::DoNextRequest(const Key_t &ask)
    {
      if(R)
      {
        parent->LookupIntroSetRecursive(target, whoasked.node, whoasked.txid,
                                        ask, R - 1);
      }
      else
      {
        parent->LookupIntroSetIterative(target, whoasked.node, whoasked.txid,
                                        ask);
      }
    }
    void
    ServiceAddressLookup::ExtractStatus(util::StatusObject &obj) const
    {
      std::vector< util::StatusObject > foundObjs;
      for(const auto &found : valuesFound)
      {
        util::StatusObject introsetObj;
        found.ExtractStatus(introsetObj);
        foundObjs.emplace_back(introsetObj);
      }
      obj.PutObjectArray("found", foundObjs);

      util::StatusObject txownerObj;
      txownerObj.PutInt("txid", whoasked.txid);
      txownerObj.PutString("node", whoasked.node.ToHex());
      obj.PutObject("whoasked", txownerObj);

      std::vector< std::string > asked;
      for(const auto &peer : peersAsked)
        asked.emplace_back(peer.ToHex());
      obj.PutStringArray("asked", asked);

      obj.PutString("target", target.ToHex());
    }

    void
    ServiceAddressLookup::SendReply()
    {
      // get newest introset
      if(valuesFound.size())
      {
        llarp::service::IntroSet found;
        for(const auto &introset : valuesFound)
        {
          if(found.OtherIsNewer(introset))
            found = introset;
        }
        valuesFound.clear();
        valuesFound.emplace_back(found);
      }
      if(handleResult)
      {
        handleResult(valuesFound);
      }
      parent->DHTSendTo(whoasked.node.as_array(),
                        new GotIntroMessage(valuesFound, whoasked.txid));
    }
  }  // namespace dht
}  // namespace llarp
